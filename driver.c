/*
 *------------------------------------------------------------------------
 * DellEMC
 *
 * Confidential and Proprietary
 * Copyright (C) 2012-2017 DellEMC All Rights Reserved
 *------------------------------------------------------------------------
 *
 * Filename: driver.c
 *
 * Author: Adrian Michaud <Adrian.Michaud@dell.com>
 *
 * File Description: Basic kernel allocator driver for TA&M 
 *
 *------------------------------------------------------------------------
 * File History
 *
 * Date     Init        Notes
 *------------------------------------------------------------------------
 * 02/15/17 Adrian      Created. 
 *------------------------------------------------------------------------
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/highmem.h>
#include <linux/mutex.h>
#include <linux/radix-tree.h>
#include <linux/buffer_head.h> /* invalidate_bh_lrus() */
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/kthread.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <asm/uaccess.h>
#include <linux/mman.h>

#include "mem_user.h"

#define MEM_DEBUG

typedef struct PCI_MEM_POOL_T PCI_MEM_POOL;
typedef struct PCI_MEM_POOL_T {
	unsigned long base_addr;
	unsigned long base_size;
	struct gen_pool *pci_memory_pool;
	PCI_MEM_POOL *next;
} PCI_MEM_POOL;

static struct PCI_MEM_POOL *pci_memory_pool_list = NULL;

static unsigned int peak_mem = 0;
static unsigned int curr_mem = 0;

#ifdef MEM_DEBUG
#define mem_debug(fmt, args...) printk(KERN_ERR "%s: " fmt "\n", __func__, ##args) 
#else
#define mem_debug(fmt, args...) 
#endif

typedef struct MEM_BLOCK_T {
	unsigned long addr;
	unsigned long size;
	struct gen_pool *pci_memory_pool;
	struct MEM_BLOCK_T *next;
	struct MEM_BLOCK_T *prev;
} MEM_BLOCK;

typedef struct MEM_USER_T {
	struct mutex lock;
	MEM_BLOCK *blocks;
} MEM_USER;

/*****************************************************************************/
/*                                                                           */
/* Function    : mem_module_setup_pool                                       */
/* Scope       : Private API                                                 */
/* Description : Setup memory pool                                           */
/* Arguments   :                                                             */
/* Returns     :                                                             */
/*                                                                           */
/*****************************************************************************/
static int mem_module_setup_pool(unsigned long base, unsigned long size)
{
	PCI_MEM_POOL *mem_pool = pci_memory_pool_list;
	
	while (mem_pool) {
		if (mem_pool->base_addr == base && mem_pool->base_size == size) {
			// Found pool, Return success
			return(0);
		}
		mem_pool = mem_pool->next;
	}
	// Create memory pool if it doesn't already exist
	if (!(mem_pool = kzalloc(sizeof(PCI_MEM_POOL), GFP_KERNEL))) {
		// Return out of memory
		return(-EINVAL);
	}
	// Debug output
	mem_debug("Creating the memory pool (base=%p, size=%p)", (void*)base, (void*)size);
	// Create PCI memory pool
	if (!(mem_pool->pci_memory_pool = gen_pool_create(PAGE_SHIFT, -1))) {
		// Return error
		return(-EINVAL);	
	}
	// Debug output
	mem_debug("Added memory to pool (base=%p, size=%p)", (void*)base, (void*)size);
	// Add physical memory to the pool
	gen_pool_add(mem_pool->pci_memory_pool, base, size, -1);

	// Add to pci memory pool list
	mem_pool->base_addr = base;
	mem_pool->base_size = size;
	mem_pool->next = pci_memory_pool_list;
	pci_memory_pool_list = mem_pool;

	// Return success
	return(0);
}

/*****************************************************************************/
/*                                                                           */
/* Function    : mem_module_allocate                                         */
/* Scope       : Private API                                                 */
/* Description : Allocate from memory pool                                   */
/* Arguments   :                                                             */
/* Returns     :                                                             */
/*                                                                           */
/*****************************************************************************/
static MEM_BLOCK *mem_module_allocate(MEM_USER *user, unsigned long base, unsigned long size)
{
MEM_BLOCK *mem;
PCI_MEM_POOL *mem_pool = pci_memory_pool_list;
	
	// Find corresponded pool
	while (mem_pool) {
		if (mem_pool->base_addr == base) {
			// Found pool
			break;
		}
		mem_pool = mem_pool->next;
	}
	if (!mem_pool) {
		// Debug output
		mem_debug("No memory Pool found (base=%p, request size=%p)", (void*)base, (void*)size);
		// Return out of memory
		return(0);
	}

	// Allocate the memory object
	if (!(mem = kzalloc(sizeof(MEM_BLOCK), GFP_KERNEL))) {
		// Return out of memory
		return(0);
	}
	// Allocate memory from pool
	if (!(mem->addr = gen_pool_alloc(mem_pool->pci_memory_pool, size))) {
		// Free memory
		kfree(mem);
		mem_debug("Out of memory (base=%p, request size=%p)", (void*)base, (void*)size);
		// Return out of memory
		return(0);
	}
	// Setup size
	mem->size = size;
	mem->pci_memory_pool = mem_pool->pci_memory_pool;
	// Debug
	mem_debug("Allocated memory (addr=%p, size=%p)", (void*)mem->addr, (void*)mem->size);
	// Return memory block
	return(mem);
}

/*****************************************************************************/
/*                                                                           */
/* Function    : mem_module_free_resource                                    */
/* Scope       : Private API                                                 */
/* Description : Free memory resource                                        */
/* Arguments   :                                                             */
/* Returns     :                                                             */
/*                                                                           */
/*****************************************************************************/
static int mem_module_free_resource(MEM_USER *user, unsigned long addr, unsigned long  size)
{
MEM_BLOCK *mem;

	// debug
	mem_debug("addr=%p, size=%p", (void*)addr, (void*)size);
	// Lock the user
	mutex_lock(&user->lock);
	while (size > 0) {
		// Setup base
		unsigned long old_size = size;
		mem = user->blocks;
		// Search for this memory block
		while(mem) {
			// Is this the correct memory block?
			if ((addr >= mem->addr && addr < (mem->addr + mem->size))) {
				// Free pool memory
				if (addr == mem->addr && size >= mem->size) {
					gen_pool_free(mem->pci_memory_pool, addr, mem->size);
					// Is this the base?
					if (user->blocks == mem) {
						// Rebase
						user->blocks = mem->next;
					} else {
						// Adjust prev link
						mem->prev->next = mem->next;
						// Is there a next?
						if (mem->next) {
							// Adjust next's prev
							mem->next->prev = mem->prev;
						}
					}
					// Free struct
					kfree(mem);
					// debug
					mem_debug("found full: addr=%p, size=%p", (void*)addr, (void*)mem->size);

					size = size - mem->size;
					addr = addr + mem->size;
					if (size > 0) {
						break;
					}
					else {
						// Unlock the user
						mutex_unlock(&user->lock);
						// Return success
						return(0);	
					}
				}
				else if (addr == mem->addr && size < mem->size) {
					gen_pool_free(mem->pci_memory_pool, addr, size);
					mem->addr = mem->addr + size;
					mem->size = mem->size - size;
					// Unlock the user
					mutex_unlock(&user->lock);
					// debug
					mem_debug("found partial from head: addr=%p, size=%p", (void*)addr, (void*)size);
					mem_debug("update mem block: addr=%p, size=%p", (void*)mem->addr, (void*)mem->size);
					// Return succes
					return(0);
				}
				else if ((addr+size) >= (mem->addr + mem->size)) {
					gen_pool_free(mem->pci_memory_pool, addr, (mem->addr + mem->size - addr));
					// debug
					mem_debug("found partial from tail: addr=%p, size=%p", (void*)addr, (void*)(mem->addr + mem->size - addr));

					size = size - (mem->addr + mem->size - addr);
					mem->size = addr - mem->addr;
					addr = mem->addr + mem->size;
					mem_debug("update mem block: addr=%p, size=%p", (void*)mem->addr, (void*)mem->size);
					if (size > 0) {
						break;
					}
					else {
						// Unlock the user
						mutex_unlock(&user->lock);
						// Return succes
						return(0);
					}
				}
				else {
					gen_pool_free(mem->pci_memory_pool, addr, size);
					MEM_BLOCK *new_mem;
					// Allocate new memory object
					if (!(new_mem = kzalloc(sizeof(MEM_BLOCK), GFP_KERNEL))) {
						// Return out of memory
						mutex_unlock(&user->lock);
						return(-EINVAL);
					}
					mem_debug("allocate new_mem obj");
					new_mem->addr = addr+size;
					new_mem->size = (mem->addr + mem->size) - (addr + size);
					mem_debug("set new_mem");
					
					mem->size = addr - mem->addr;
					// Connect new memory object
					new_mem->next = mem->next;
					if (mem->next) {
						mem->next->prev = new_mem;
					}
					new_mem->prev = mem;
					mem->next = new_mem;
					new_mem->pci_memory_pool = mem->pci_memory_pool;

					// Unlock the user
					mutex_unlock(&user->lock);
					// debug
					mem_debug("found partial in the middle: addr=%p, size=%p", (void*)addr, (void*)size);
					// Return success
					return(0);
				}
			}
			// Move to next
			mem = mem->next;
		}
		// We didn't find any candidate block to free, return error
		if (size == old_size) break;
	}
	
	// debug
	mem_debug("error: addr=%p, size=%p", (void*)addr, (void*)size);
	// Unlock the user
	mutex_unlock(&user->lock);
	// Return error
	return(-EINVAL);
}


/*****************************************************************************/
/*                                                                           */
/* Function    : mem_module_add_resource                                     */
/* Scope       : Private API                                                 */
/* Description : Record resource for this user.                              */
/* Arguments   :                                                             */
/* Returns     :                                                             */
/*                                                                           */
/*****************************************************************************/
static void mem_module_add_resource(MEM_USER *user, MEM_BLOCK *mem)
{
	// Lock the user
	mutex_lock(&user->lock);
	// Is this the first block?
	if (!user->blocks) {
		// Setup base
		user->blocks = mem;
	} else {
		// Adjust prev
		user->blocks->prev = mem;
		// Setup next
		mem->next = user->blocks;
		// Adjust base
		user->blocks = mem;
	}
	// Unlock the user
	mutex_unlock(&user->lock);
	// debug
	mem_debug("added addr=%p, size=%p", (void*)mem->addr, (void*)mem->size);
}

/*****************************************************************************/
/*                                                                           */
/* Function    : mem_module_unlocked_ioctl                                   */
/* Scope       : Private API                                                 */
/* Description : Provides the IOCTL interface.                               */
/* Arguments   :                                                             */
/* Returns     :                                                             */
/*                                                                           */
/*****************************************************************************/
static long mem_module_unlocked_ioctl(struct file *filp, unsigned int command, unsigned long arg)
{
MEM_REQUEST request;
MEM_BLOCK *mem;
MEM_USER *user;

	// Release resources
	user = (MEM_USER *)filp->private_data;

	// Process user IOCTL command
	switch (command) {
		// Alloc
		case DRIVER_IOCTL_ALLOC:
			// Get request
			if (copy_from_user(&request, (void __user *) arg, sizeof(request))) {
				// Log error
				mem_debug("FAULT reading request");
				// Return error
				return(-EFAULT);
			}
			// Create memory pool if it doesn't already exist
			if (mem_module_setup_pool(request.bar_base, request.bar_size)) {
				// Log error
				mem_debug("failed to create PCI memory pool");
				// Return error
				return(-EINVAL);	
			}
			// Allocate the memory object
			if (!(mem = mem_module_allocate(user, request.bar_base, request.alloc_size))) {
				// Debug
				mem_debug("Failed to allocate block memory");
				// Return out of memory
				return(-ENOMEM);
			}
			// Setup address
			request.alloc_addr = mem->addr;
			// Copy results back
			if (copy_to_user((void __user *)arg, &request, sizeof(request))) {
				// Log error
				mem_debug("FAULT writing request data");
				// Deallocate memory
				kfree(mem);
				// Return error
				return(-EFAULT);
			}
			// Add resource
			mem_module_add_resource(user, mem);
			// Mem counter
			curr_mem += mem->size;
			peak_mem = peak_mem > curr_mem ? peak_mem : curr_mem;
			// return success
			return(0);

		// Free
		case DRIVER_IOCTL_FREE:
			// Get request
			if (copy_from_user(&request, (void __user *) arg, sizeof(request))) {
				// Log error
				mem_debug("FAULT reading request");
				// Return error
				return(-EFAULT);
			}
			// Free request
			if (mem_module_free_resource(user, request.alloc_addr, request.alloc_size)) {
				// Log error
				mem_debug("Failed to free resource");
				// Return error
				return(-EINVAL);	
			}
			// Mem counter
			curr_mem -= request.alloc_size;
			// return success
			return(0);

		// Rest Mem counter
		case DRIVER_IOCTL_RESET_CNT:
			curr_mem = 0;
			peak_mem = 0;
			// return success
			return(0);
	}
	// Unknown MCA IOCTL command
	return(-EINVAL);
}

/*****************************************************************************/
/*                                                                           */
/* Function    : mem_module_open                                             */
/* Scope       : Private API                                                 */
/* Description :                                                             */
/* Arguments   :                                                             */
/* Returns     :                                                             */
/*                                                                           */
/*****************************************************************************/
static int mem_module_open(struct inode *inode, struct file *filp)
{
MEM_USER *user;

	// Allocate instance data
	if (!(user = kzalloc(sizeof(MEM_USER), GFP_KERNEL))) {
		// Display error
		return(-ENOMEM);
	}
	// Init lock
	mutex_init(&user->lock);
	// Setup instance data
	filp->private_data = user;
	// Return success
	return(0);
}

/*****************************************************************************/
/*                                                                           */
/* Function    : mem_module_mmap                                             */
/* Scope       : Private API                                                 */
/* Description :                                                             */
/* Arguments   :                                                             */
/* Returns     :                                                             */
/*                                                                           */
/*****************************************************************************/
static int mem_module_mmap(struct file *filp, struct vm_area_struct *vma)
{
	//pgprot_noncached(vma->vm_page_prot);
	vma->vm_page_prot = __pgprot_modify(vma->vm_page_prot, PTE_ATTRINDX_MASK, PTE_ATTRINDX(MT_NORMAL) | PTE_PXN | PTE_UXN);
	//vma->vm_page_prot = __pgprot_modify(vma->vm_page_prot, PTE_ATTRINDX_MASK, PTE_ATTRINDX(MT_NORMAL) | PTE_PXN | PTE_UXN);
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot))
	{
		return -EAGAIN;
	}
	mem_debug("mem_module_mmap succeed, va: %p, size: %p, pa: %p\n",(void *)vma->vm_start, (void *)(vma->vm_end - vma->vm_start),(void *)(vma->vm_pgoff<<PAGE_SHIFT));
	return 0;
}

/*****************************************************************************/
/*                                                                           */
/* Function    : mem_module_release                                          */
/* Scope       : Private API                                                 */
/* Description :                                                             */
/* Arguments   :                                                             */
/* Returns     :                                                             */
/*                                                                           */
/*****************************************************************************/
static int mem_module_release(struct inode *inode, struct file *filp)
{
MEM_USER *user;
MEM_BLOCK *mem, *next;

	// Release resources
	if ((user = (MEM_USER *)filp->private_data)) {
		// Lock the user
		mutex_lock(&user->lock);
		// Setup base
		mem = user->blocks;
		// Dealloc all
		while(mem) {
			// Setup next
			next = mem->next;
			// debug
			mem_debug("Freeing addr=%p, size=%p", (void*)mem->addr, (void*)mem->size);
			// Free pool memory
			gen_pool_free(mem->pci_memory_pool, mem->addr, mem->size);
			// Free struct
			kfree(mem);
			// Move to next
			mem = next;
		}
		// Unlock the user
		mutex_unlock(&user->lock);
		// Free user
		kfree(user);
	}
	// Print & Reset mem counter
	mem_debug("Peak memory footprint size=%u Byte", peak_mem);
	peak_mem = 0;
	curr_mem = 0;
	// Return success
	return(0);
}

static const struct file_operations mem_module_fops = {
	.owner			= THIS_MODULE,
	.unlocked_ioctl	= mem_module_unlocked_ioctl,
	.release        = mem_module_release,
	.open           = mem_module_open,
	.mmap           = mem_module_mmap,

};

static struct miscdevice mem_module_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = MODULE_DEV_NODE,
	.mode  = S_IRUGO|S_IWUGO,
	.fops  = &mem_module_fops,
};

/*****************************************************************************/
/*                                                                           */
/* Function    : mem_module_exit                                             */
/* Scope       : Private API                                                 */
/* Description :                                                             */
/* Arguments   :                                                             */
/* Returns     :                                                             */
/*                                                                           */
/*****************************************************************************/
static void __exit mem_module_exit(void)
{
	PCI_MEM_POOL *mem_pool = pci_memory_pool_list;
	
	// Find corresponded pool
	while (mem_pool) {
		// Is there a memory pool?
		if (mem_pool->pci_memory_pool) {
			// log message
			printk(KERN_ERR "freeing memory pool (base=%p, size=%p)\n", (void*)mem_pool->base_addr,(void*)mem_pool->base_size);
			// Deallocate it
			gen_pool_destroy(mem_pool->pci_memory_pool);
		}
		PCI_MEM_POOL *next_mem_pool = mem_pool->next;
		kfree(mem_pool);
		mem_pool = next_mem_pool;
	}
	
	// Unregister control device
	misc_deregister(&mem_module_dev);
	// Emit kernel log message
	printk(KERN_ERR "%s: unloaded\n", MODULE_DEV_NODE);
}

/*****************************************************************************/
/*                                                                           */
/* Function    : mem_module_init                                             */
/* Scope       : Private API                                                 */
/* Description :                                                             */
/* Arguments   :                                                             */
/* Returns     :                                                             */
/*                                                                           */
/*****************************************************************************/
static int __init mem_module_init(void)
{
	// Register device control
	if (misc_register(&mem_module_dev)) {
		// Failed to register driver
		printk(KERN_ERR "failed to register device control");
		// Return error
		return(-ENODEV);
	}
	// Emit kernel log message
	printk(KERN_ERR "%s: loaded\n", MODULE_DEV_NODE);
	// Return success
	return(0);
}

module_init(mem_module_init);
module_exit(mem_module_exit);

MODULE_AUTHOR("Adrian Michaud <Adrian.Michaud@dell.com>");
MODULE_DESCRIPTION("Memory Driver");
MODULE_LICENSE("GPL");
