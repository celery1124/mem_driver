#
# ------------------------------------------------------------------------
#  DellEMC
# 
#  Confidential and Proprietary
#  Copyright (C) 2012-2017 DellEMC All Rights Reserved
# ------------------------------------------------------------------------
# 
#  Filename: Makefile
# 
#  Description: Makefile for basic a allocator driver for TA&M
# 
#  Author: Adrian Michaud <Adrian.Michaud@dell.com>
# 
# ------------------------------------------------------------------------
#  File History
# 
#  Date     Init        Notes
# ------------------------------------------------------------------------
#  02/15/17 Adrian      Created.
# ------------------------------------------------------------------------
#

EXTRA_CFLAGS += -O3 -Wall

mem_driver-objs := driver.o

obj-m := mem_driver.o

PWD = $(CURDIR)
KERNELDIR ?= /home/celery/yocto_sdk/ISSD-SDK-20160701-yocto/LS2085A-SDK-20160304-yocto/build_ls2085aissd_release/tmp/work/ls2085aissd-fsl-linux/linux-ls2-sdk/4.1-r0/build/

default:
	@$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	#@install -d $(PREFIX)/lib/modules/$(shell uname -r)/kernel/drivers/char
	#@install *.ko $(PREFIX)/lib/modules/$(shell uname -r)/kernel/drivers/char
	#@/sbin/depmod -a

clean:
	@echo "Cleaning Memory Driver"
	@$(MAKE) -s -C $(KERNELDIR) M=$(PWD) clean
