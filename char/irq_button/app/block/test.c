#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
int main(int argc ,char *argv[])
{
	int fd;
	unsigned char key_val;
	fd = open("/dev/x4412-btn",O_RDWR);
	if (fd < 0)
	{
		printf("open error\n");
		return fd;
	}
	
	while(1)
	{
		read(fd,&key_val,1);
		printf("key_val = 0x%x\n",key_val);
//		sleep(500);
	}
	return 0;
}