# mem_driver for EMC project

## Description

This driver accomplish two things. First, manage chunk/extent allocation requests from jemalloc/memkind on the PCIe memory. This is done through kernel gen_pool_alloc module and a linked-list of allocated chunks.  Second, this driver make sure the allocated (mmap) memory to be cacheable for host.

## Build

```bash
ARCH=arm64 CROSS_COMPILE=$cross_compiler_path/aarch64-fsl-linux- make
```

example:

```bash
ARCH=arm64 CROSS_COMPILE=/home/celery/yocto_sdk/ISSD-SDK-20160701-yocto/LS2085A-SDK-20160304-yocto/build_ls2085aissd_release/tmp/sysroots/x86_64-linux/usr/bin/aarch64-fsl-linux/aarch64-fsl-linux- make
```

## Usage

```bash
insmod mem_driver.ko
rmmod mem_driver.ko

#log
dmesg
```
