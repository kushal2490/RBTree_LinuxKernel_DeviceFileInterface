Part 1 - Accessing a kernel RB tree via device file interface
============================================================================================
It contains 4 files:
user_rbt530.c
rbt530.h
libioctl.h
rbt530_drv.c
Makefile

user_rbt530.c	->	This is a user space app to test kernel driver rbt530_drv to access a kernel RB Tree device. It defines the operations (read, write, set_end, dumptree) to perform on the device tree.
The app spawns 4 threads and initiates the RB-tree with 40 objects.
After writing 40 objects the multithreaded app performs 100 random R/W file operations on the device rb-tree. After all the threads exit, the main thread prints out all the nodes present in the RB tree.

rbt530_drv.c	->	This is a device driver implemented to perform file operations on the rbt530_dev device to augment the tree nodes . It implements the ioctl commands and read/write operations.
Read operation 	- Displays the First or Last object node of the tree based on the flag set by the user app.
Write operation - Insert a node into the tree if it has a key and data pair, if data is not zero.
set_end 		- This command is used to set the reader flag to 0 or 1.
SETDUMP 		- This command is used to set the dump flag to 1 for dumping the tree nodes.
PRINT 			- Can be used to print the completion message of 40 objects written to tree.

rbt530.h 		->	This contains 1. rb_object structure containing the key, data members for use by the kernel device and user app 2. dump structure containing information of the tree (number of nodes, and an array to hold all nodes) for use when the RB-tree nodes need to be printed

libioctl.h		->	This contains the ioctl commands definition.

Makefile		->	To make the target object file of the i2cflash driver.

run.sh			->	This script:- 
					1) Clears the dmesg logs
					2) installs the kernel module "rbt530_drv.ko"
					3) runs the executable "app" and stores the user logs in userOutput.txt
					4) saves the kernel messages for the device operations in kernelOutput.txt

----------------------------------------------------------------------------------------------------------------------------------
Compilation & Usage Steps:
=========================================================================================
Inorder to compile the driver kernel object file:
----------------------------------------------------------------------------------------------------------------------------------

NOTE:
1) Remove any prior rbt530_dev dev node from the the Galileo Gen2 board.
rmmod rbt530_drv.ko

2) Export your SDK's toolchain path to your current directory to avoid unrecognized command.
export PATH="/opt/iot-devkit/1.7.3/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux/:$PATH"
=================================================================================================

Method:-

1) Change the following KDIR path to your source kernel path and the SROOT path to your sysroots path in the Makefile
KDIR:=/opt/iot-devkit/1.7.3/sysroots/i586-poky-linux/usr/src/kernel
SROOT=/opt/iot-devkit/1.7.3/sysroots/i586-poky-linux/

2) Run make command to compile both user app and the rbt530_dev kernel object.
make

3) copy 3 files from current dir to board dir using the below command:
	i) 	rbt530_drv.ko
	ii)	app
	iii)run.sh

sudo scp <filename> root@<inet_address>:/home/root

4) On the Galileo Gen2 board, give executable permissions to run.sh script:
chmod 777 run.sh

5) Run the script "run.sh" 
./run.sh

6) Twp Output files are generated
	i) kernelOutput.txt	--> contains the kernel logs
	ii)userOutput.txt	--> contains the User logs

7) Copy the log files to user using the below command
scp root@<inet_address>:/home/root/userOutput.txt .
scp root@<inet_address>:/home/root/kernelOutput.txt .
=======================================================================================================================================================================================
