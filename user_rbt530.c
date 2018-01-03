#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/errno.h>
#include <sched.h>
#include <math.h>
#include <sys/ioctl.h>
#include "libioctl.h"
#include "rbt530.h"

#define DEVICE "/dev/rbt530_dev"

pthread_mutex_t mutex;

unsigned int rwcount = 0;
unsigned int wcount = 0;
int errno, fd, wdone = 0;

/* 
* Function to fill rb-tree object with key and data 
*	--> Fills incrementing (key,data) values for first 40 writes
*	--> Fills (key,data) supplied by the random R/W file operations
*/
rb_object_t user_datafill(int key, int value)
{
	static int data = 0;
	static int k = 0;
	rb_object_t user_object;
	
	if(key == 0 && value == 0)
	{
		data +=1;
		value=data;

		k+=1;
		key=k;
	}
	user_object.key = key;
	user_object.data = value;
	return user_object;
}

/*
* Thread function called by 4 concurrent threads 
* spawned by the main thread 
*/
void *rbops_t_func(void *ptr)
{
	int ret;
	rb_object_t userBuffer, kernelBuffer;
	int key, data, RW, FL, time_us;
	
	/* 40 writes to an empty rb-tree */
	while(wcount < 40 && wdone ==0)
	{
		time_us = rand() % 10000 + 100;		//Generate random delay for sleep period
		userBuffer = user_datafill(0, 0);

		pthread_mutex_lock(&mutex);
		if(wcount<40){
			ret = write(fd, &userBuffer, sizeof(rb_object_t));
			if(ret!=0)
				printf("\nWrite operation failed.\n");
			wcount++;
		}
			/* Print for completion of 40 objects written to the tree */
		// if((wcount >= 40) && (wdone++==0))
		// {
		// 	ioctl(fd, PRINT, 0);
		// 	printf("\n40 OBJECTS WRITTEN. STARTING 100 RANDOM READ/WRITE OPERATIONS\n\n");
		// 	//wdone++;
		// }
		pthread_mutex_unlock(&mutex);
		printf("\nInserting NODE: k=%d v=%d\n", userBuffer.key, userBuffer.data);
		usleep(time_us);
	}
	
	/* 100 random R/W file operations */
	if(wcount >= 40)
	{
		while(rwcount < 100)
		{	RW = rand() % 2;				//generate 0 or 1 to specify READ or WRITE file operation
			FL = rand() % 2;				//generate 0 or 1 to read the FIRST or LAST object node	
			key = rand() % 100; 			// generate a random key from 0 to 100
			data = rand() % 100 + 42; 		// generate a random data from 0 to 100
			time_us = rand() % 10000 + 100;	//generate random delay for sleep period

			userBuffer = user_datafill(key, data);
			if(RW){		//RW = 1 denotes Write operation
					pthread_mutex_lock(&mutex);
					if(rwcount<100){
						ret = write(fd, &userBuffer, sizeof(rb_object_t));
						if(ret!= 0)
						{
							printf("\nWrite operation failed.%d\n", ret);	
						}
						rwcount++;
					}
					pthread_mutex_unlock(&mutex);
					printf("\nWrite Request: k=%d v=%d\n", userBuffer.key, userBuffer.data);
					usleep(time_us);
			}
			else{		//RW = 0 denotes READ operation
				pthread_mutex_lock(&mutex);
				if(rwcount<100){
					ret = read(fd, &kernelBuffer, sizeof(rb_object_t));
					rwcount++;
					pthread_mutex_unlock(&mutex);
					printf("Reading End %d of Tree: k=%d, v=%d\n", FL, kernelBuffer.key, kernelBuffer.data);
					if(ret!= 0)
					{
						printf("\nRead operation failed. %d\n", errno);	
					}
					/* command to set the reading end at First node or Last node */
					ret = ioctl(fd, SETEND, FL);
					if(ret!= 0)
					{
						printf("\nIOCTL failure %d\n", ret);	
					}
				}
				else {
					pthread_mutex_unlock(&mutex);
				}			

				usleep(time_us);
			}
		}
	}
	return NULL;
}

/* function to dump all the elements of the rb-tree device */
void dumptree()
{	
	struct dump kernelBuffer;
	int num = 1, i, ret;
	int numNodes;

	ret = ioctl(fd, SETDUMP, 1);
	if(ret!= 0)
	{
		printf("\nIOCTL failure %d\n",ret);	
	}

	ret = read(fd, &kernelBuffer, sizeof(struct dump));

	numNodes = kernelBuffer.numNodes;
	printf("\n **** DUMPING TREE **** \n\n");
	for(i=0;i<numNodes;i++){
		printf("node %d: k=%d  v=%d\n", num++, kernelBuffer.dumparray[i].key, kernelBuffer.dumparray[i].data);
	}

}

int main()
{
	int ret, i=0, j=0;
	const int thread_priority[] = {94,95,96,97};
	struct sched_param param;
	int *arg = malloc(sizeof(int));
	*arg = 0;
	pthread_attr_t tattr[4];
	pthread_t rbops_t[4];

	fd = open(DEVICE, O_RDWR);
	if(fd == -1){
		printf("Cannot open device  %d\n", fd);
		exit(-1);
	}

	if(pthread_mutex_init(&mutex, NULL) != 0){
		printf("\n mutex init failed\n");
	}

	/* Initialize 4 threads with the given attributes */
	while(i<4){
		
		/* initialized with default attributes */
		ret = pthread_attr_init (&(tattr[i]));
		if(ret!=0)
		{
			printf("pthread_attr_init failed. %d\n", ret);
		}

		pthread_attr_setschedpolicy(&(tattr[i]), SCHED_FIFO);

		/* safe to get existing scheduling param */
		ret = pthread_attr_getschedparam (&(tattr[i]), &param);
		if(ret!=0)
		{
			printf("pthread_attr_getschedparam failed. %d\n", ret);
		}

		/* set the priority; others are unchanged */
		param.sched_priority = thread_priority[i];

		*arg += 100;
		
		/* setting the new scheduling param */
		ret = pthread_attr_setschedparam (&(tattr[i]), &param);
		if(ret!=0)
		{
			printf("pthread_attr_setschedparam failed. %d\n", ret);
		}

		/*  sets the inherit-scheduler attribute of the thread attributes object referred to by attr to the value specified in inheritsched. */
		ret = pthread_attr_setinheritsched(&(tattr[i]), PTHREAD_EXPLICIT_SCHED);
		if(ret!=0)
		{
			printf(" pthread_attr_setinheritsched failed. %d\n", ret);
		}

		/* Create the threads*/
		ret = pthread_create(&(rbops_t[i]), &(tattr[i]), rbops_t_func, arg);
		if(ret!=0)
			printf("Thread creation failed %d\n", ret);

		i++;
	}

	while(j<4){
		pthread_join(rbops_t[j++], NULL);
	}

	/* function to dump tree elements */
	dumptree();

	ret = close(fd);
	if(ret!=0)
		printf("Cannot close device  %d\n", ret);

	pthread_mutex_destroy(&mutex);
	return 0;
}


