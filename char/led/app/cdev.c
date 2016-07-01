#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#define DEVICE_NAME "/dev/x4412-cdev"
int main(int argc,char **argv)
{
	int fd;
	char buf[30];
	char str[]="hello,x4412!";
	fd = open(DEVICE_NAME,O_RDWR);
	if(fd == -1)
	{
		printf("open device %s error \n",DEVICE_NAME);
	}
	else
	{
		read(fd,buf,30);
		printf("read from kernel: %s\r\n",buf);
		usleep(5000);
		printf("write value to kernel:\r\n");
		usleep(5000);
		write(fd,str,12);
		close(fd);
	}
	return 0;
}
