rmmod rbt530_drv.ko
dmesg -c
insmod rbt530_drv.ko
./app > userOutput.txt
dmesg > kernelOutput.txt