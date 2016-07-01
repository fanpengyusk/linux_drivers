#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#define DEVICE_NAME "/dev/x4412-globalmem"
#define MEM_CLEAR 0x1
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
		write(fd,str,sizeof(str));
		usleep(5000);
		lseek(fd,0,SEEK_SET);
		read(fd,buf,sizeof(str));
//		printf("read from /dev/x4412-gloalmem:\r\n");
		printf("buf:%s\r\n",buf);
		usleep(5000);
		ioctl(fd,MEM_CLEAR);
		
		lseek(fd,0,SEEK_SET);
		read(fd,buf,sizeof(str));
//		printf("read from /dev/x4412-gloalmem later:\r\n");
		printf("%s\r\n",buf);
		close(fd);
	}
	return 0;
}