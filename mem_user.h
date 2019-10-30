/*
 *------------------------------------------------------------------------
 * DellEMC
 *
 * Confidential and Proprietary
 * Copyright (C) 2012-2017 DellEMC All Rights Reserved
 *------------------------------------------------------------------------
 *
 * Filename: mem_user.h
 *
 * Author: Adrian Michaud <Adrian.Michaud@dell.com>
 *
 * File Description: Userland interface to kernel allocator driver for TA&M 
 *
 *------------------------------------------------------------------------
 * File History
 *
 * Date     Init        Notes
 *------------------------------------------------------------------------
 * 02/15/17 Adrian      Created. 
 *------------------------------------------------------------------------
 */

#ifndef __MEM_USER_H
#define __MEM_USER_H 

#ifdef __cplusplus
extern "C" {
#endif

#define MODULE_DEV_NODE  "mem_driver"

#define DRIVER_IOCTL_ALLOC _IO( 0xac, 100)
#define DRIVER_IOCTL_FREE  _IO( 0xac, 101)

typedef struct MEM_REQUEST_T {
	unsigned long bar_base;    // In:  PCI BAR address (bytes)
	unsigned long bar_size;    // In:  PCI BAR size    (bytes)
	unsigned long alloc_size;  // In:  size of request (bytes)
	unsigned long alloc_addr;  // Out: alloc address
} MEM_REQUEST;

#ifdef __cplusplus
}
#endif

#endif // __MEM_USER_H

