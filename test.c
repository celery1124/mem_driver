/*
 *------------------------------------------------------------------------
 * DellEMC
 *
 * Confidential and Proprietary
 * Copyright (C) 2012-2017 DellEMC All Rights Reserved
 *------------------------------------------------------------------------
 *
 * Filename: test.c
 *
 * Author: Adrian Michaud <Adrian.Michaud@dell.com>
 *
 * File Description: Basic kernel allocator test
 *
 *------------------------------------------------------------------------
 * File History
 *
 * Date     Init        Notes
 *------------------------------------------------------------------------
 * 02/15/17 Adrian      Created. 
 *------------------------------------------------------------------------
 */
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <stdarg.h>

#include "mem_user.h"

#define BAR2_ADDR 0x1280000000

#define BAR2_SIZE (1024L*1024L*1024L)  // 1GB

int main(void)
{
int rc;
int fd;
int i;
MEM_REQUEST request[4];
char string[128];

	// Open up memory driver
	if ((fd = open("/dev/mem_driver", O_RDWR)) < 0) {
		// We failed to open the device
		printf("Failed to open /dev/mem_driver\n");
		// Fail
		return(-1);
	}

	// Loop to alloc 4 blocks
	for (i=0; i<4; i++) {
		// Setup memory request using fixed BAR addr and size
		request[i].bar_base   = BAR2_ADDR;
		request[i].bar_size   = BAR2_SIZE;
		// Setup the allocation request size (must be rounded to pages)
		request[i].alloc_size = 32*4096;
		// Allocate a memory block from the driver
		rc = ioctl(fd, DRIVER_IOCTL_ALLOC, &request[i]);
		// Print results
		printf("ALLOC Return value: rc=%d, addr=%p, size=%p\n", rc, request[i].alloc_addr, request[i].alloc_size);
	}

	printf("Waiting for you to press return... Run the program in another process to allocate more memory:\n");
	fgets(string, sizeof(string)-1, stdin);

	// Loop to free 4 blocks. Or just exit the process and the
	// blocks will automatically get deallocated by the mem_driver
	for (i=0; i<4; i++) {
		// Free a memory block back to the driver
		rc = ioctl(fd, DRIVER_IOCTL_FREE, &request[i]);
		// Print results
		printf("FREE Return value: rc=%d, addr=%p, size=%p\n", rc, request[i].alloc_addr, request[i].alloc_size);
	}
	// Close mem_driver handle
	close(fd);
}
