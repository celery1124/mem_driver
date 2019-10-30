Switch to root if not already root

$sudo su root

Build mem_driver:

$make

Install mem_driver:

$insmod ./mem_driver.ko

Build test application:

$gcc -o test test.c

Run test application:

$./test

ALLOC Return value: rc=0, addr=0x1280000000, size=0x20000
ALLOC Return value: rc=0, addr=0x1280020000, size=0x20000
ALLOC Return value: rc=0, addr=0x1280040000, size=0x20000
ALLOC Return value: rc=0, addr=0x1280060000, size=0x20000
Waiting for you to press return... Run the program in another process to allocate more memory:

<Press enter, or run the test application again in another shell to allocate more memory>

FREE Return value: rc=0, addr=0x1280000000, size=0x20000
FREE Return value: rc=0, addr=0x1280020000, size=0x20000
FREE Return value: rc=0, addr=0x1280040000, size=0x20000
FREE Return value: rc=0, addr=0x1280060000, size=0x20000
$



The test application makes an ioctl call to the mem_driver to allocate and free memory blocks. 
You can then proceed to call mmap on /dev/mem with the address (mmap offset). The mem_driver
doesn't actually allocate any memory, it's just mananing the pool of memory that you provide.
See the test.c code for more details.

Adrian
 


