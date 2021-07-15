# mem_driver for EMC project

## Description

This driver accomplish two things. First, manage chunk/extent allocation requests from jemalloc/memkind on the PCIe memory. This is done through kernel gen_pool_alloc module and a linked-list of allocated chunks.  Second, this driver make sure the allocated (mmap) memory to be cacheable for host.

For more details, please read our papers [HMMU](https://cesg.tamu.edu/wp-content/uploads/2012/02/TCAD3012213-Hardware-Memory-Mgmt-for-Future-Mobile-Hybrid-Memory-Systems.pdf) and [OpenMem](https://cesg.tamu.edu/wp-content/uploads/2012/02/OpenMem-Hardware-Software-Cooperative-Management-for-Mobile-Memory-System.pdf).

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

## Cite our work

```bibtex
@ARTICLE{Wen-HMMU,
  author={Wen, Fei and Qin, Mian and Gratz, Paul V. and Reddy, A. L. Narasimha},
  journal={IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems}, 
  title={Hardware Memory Management for Future Mobile Hybrid Memory Systems}, 
  year={2020},
  volume={39},
  number={11},
  pages={3627-3637},
  doi={10.1109/TCAD.2020.3012213}}

```
