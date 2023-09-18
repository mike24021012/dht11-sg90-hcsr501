#include <stdio.h>
#include <stdlib.h>	//system
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include<pthread.h>

//device name
#define DEVICE_NAME_D "/dev/dht11"
#define DEVICE_NAME_S "/dev/sg90"
#define DEVICE_NAME_H "/dev/hcsr501"

//ioctl
#define MAGIC_NUMBER 'X'
#define PWM_IOCTL_SET_FREQ	_IO(MAGIC_NUMBER,  1)

//file descriptor
int dht11_file,sg90_file,hcsr501_file;

//thread
pthread_t dht11_thread_t,sg90_thread_t,hcsr501_thread_t;

//global variables used between threads communication
//dht11
int dht11_state=2;               //state_value   0:thread was used before, now close 
				//			 1:thread is continuing 
				// 			 2:thread never been used 
//sg90
int sg90_state=2,sg90_rotate_direction=0,sg90_speed;
char sg90_speed_level='1';
//hcsr501 
int hcsr501_state=2;
char received_buf[1]={'0'};

void* sg90_run_thread(void* arg)
{
	while(1)
	{
		if(sg90_rotate_direction==1)
		{
			sg90_speed = 14 - (sg90_speed_level - 48);
		}
		else
		{	
			sg90_speed = 16 + (sg90_speed_level - 48);
		}
		
		ioctl(sg90_file, PWM_IOCTL_SET_FREQ, sg90_speed);
		
		if(sg90_state==0)
			break;
	}
	pthread_exit(NULL);
}

void* dht11_run_thread(void* arg)
{
	int ret = 0;
	char buf[5];
	unsigned char  tempz = 0;
	unsigned char  tempx = 0;
	unsigned char  humidiyz = 0;
	unsigned char  humidiyx = 0;
	
	while(1)
	{
		ret = read(dht11_file,buf,sizeof(buf));
    		if(ret < 0)
    		{
    			printf("read err!\n");
		}
        	else
		{
			humidiyz	=buf[0];
			humidiyx	=buf[1];
			tempz		=buf[2];
			tempx		=buf[3];
			printf("humidity = %d.%d%%\n", humidiyz, humidiyx);
			printf("temperature = %d.%d\n", tempz, tempx);	
        	}
		sleep(2);
		
		if(tempz < 30)
			sg90_speed_level='3';
		else if(tempz <= 35)
			sg90_speed_level='5';		
		else
			sg90_speed_level='8';
		
		if(dht11_state==0)
			break;
	}
	pthread_exit(NULL);
}

void* hcsr501_run_thread(void* arg)
{
	int respons;
	while(1)
	{
		respons = read(hcsr501_file,received_buf,sizeof(received_buf));
    		if(respons < 0)
    		{
    			printf("read err!\n");
		} 
		
		if(received_buf[0]=='1')
		{
			printf("Detect somebody.\n");
			if(sg90_rotate_direction==1)
				sg90_rotate_direction=0;
			else
				sg90_rotate_direction=1; //positive rotate
		}
		else
		{
			printf("Detect none.\n");
		}
		
		sleep(5);
		if(hcsr501_state==0)
			break;
	}
	pthread_exit(NULL);
}


int main(void)
{
	int res;
	char mycmd;

	dht11_file = open(DEVICE_NAME_D, O_RDONLY);
	if(dht11_file < 0)
	{
		perror("can not open dht11 device");
		exit(1);
	}
	
	sg90_file = open(DEVICE_NAME_S, O_RDWR);
	if(sg90_file < 0)
	{
		perror("can not open sg90 device");
		exit(1);
	}

	hcsr501_file = open(DEVICE_NAME_H, O_RDONLY);
	if(sg90_file < 0)
	{
		perror("can not open sg90 device");
		exit(1);
	}
	
	while(1)
	{
	        printf("Input command, 1:start dht11 2:start sg90 3:start hcsr501 4:close dht11 5:close sg90 6:close hcsr501 7:leave program.\n");
		scanf("%c",&mycmd);
		getchar();
		switch(mycmd)
		{
		case '1':
			printf("start dht11_pthread.\n");
			dht11_state=1;
			res=pthread_create(&dht11_thread_t,NULL,dht11_run_thread,NULL);
			if(res!=0)
			{
				printf("dht11 thread create failed");
				exit(1);
			}
			continue;
		case '2':		
			printf("start sg90_pthread.\n");
			dht11_state=1;
			res=pthread_create(&sg90_thread_t,NULL,sg90_run_thread,NULL);
			if(res!=0)
			{
				printf("sg90 thread create failed");
				exit(1);
			}
			continue;
    		case '3':
			printf("start hcsr501_pthread.\n");
			dht11_state=1;
			res=pthread_create(&hcsr501_thread_t,NULL,hcsr501_run_thread,NULL);
			if(res!=0)
			{
				printf("hcsr501 thread create failed");
				exit(1);
			}
			continue;
		case '4':
			if(dht11_state==1)
			{
				dht11_state=0;
				res=pthread_join(dht11_thread_t,NULL);
				if(res!=0)
				{
					printf("dht11_run_thread join failed\n");
					exit(1);
				}
				printf("dht11_run_thread join finished.\n");
			}
			printf("dht11 thread close.\n");
			continue;
		case '5':
			if(sg90_state==1)
			{
				sg90_state=0;
				res=pthread_join(sg90_thread_t,NULL);
				if(res!=0)
				{
					printf("sg90_run_thread join failed\n");
					exit(1);
				}
				printf("sg90_run_thread join finished.\n");
			}
			printf("sg90 thread close.\n");
			continue;
		case '6':
			if(hcsr501_state==1)
			{
				hcsr501_state=0;
				res=pthread_join(hcsr501_thread_t,NULL);
				if(res!=0)
				{
					printf("hcsr501_run_thread join failed\n");
					exit(1);
				}
				printf("hcsr501_run_thread join finished.\n");
			}
			printf("hcsr501 thread close.\n");
			continue;
		case '7':
			if(dht11_state==1)
			{
				dht11_state=0;
				res=pthread_join(dht11_thread_t,NULL);
				if(res!=0)
				{
					printf("dht11_run_thread join failed\n");
					exit(1);
				}
				printf("dht11_run_thread join finished.\n");
			}
			if(sg90_state==1)
			{
				sg90_state=0;
				res=pthread_join(sg90_thread_t,NULL);
				if(res!=0)
				{
					printf("sg90_run_thread join failed\n");
					exit(1);
				}
				printf("sg90_run_thread join finished.\n");
			}
			if(hcsr501_state==1)
			{
				hcsr501_state=0;
				res=pthread_join(hcsr501_thread_t,NULL);
				if(res!=0)
				{
					printf("hcsr501_run_thread join failed\n");
					exit(1);
				}
				printf("hcsr501_run_thread join finished.\n");
			}
			printf("threads all close.\n");
			break;
		default:
			printf("input wrong command, please input again.\n");
			continue;
		}
		break;	
	}
	
	if (dht11_file >= 0)	 //close dht11 if open
	{
		close(dht11_file);
		printf("close file success.\n");
	}
	
	if (sg90_file >= 0)	 //close sg90 if open
	{
		close(sg90_file);
		printf("close file success.\n");
	}

	if (hcsr501_file >= 0)	 //close hcsr501 if open
	{
		close(hcsr501_file);
		printf("close file success.\n");
	}
	
	return 0;
}
