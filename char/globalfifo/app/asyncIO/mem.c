#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
void input_handler(int signum)
{
	printf("receive a signal from globalfifo,signalnum:%d\n", signum);
}

int main()
{
	int oflags, fd;
	
	fd = open("/dev/x4412-globalfifo", O_RDWR);
	
	if (fd != -1)
	{
		signal(SIGIO,input_handler);//让 input_handler 处理 SIGIO 信号
		fcntl(fd,F_SETOWN,getpid());
		oflags = fcntl(fd,F_GETFL);
		fcntl(fd,F_SETFL,oflags | FASYNC);
		while(1)
		{
			sleep(1000);
		}
	}else
	{
		printf("open /dev/x4412-globalfifo failed");
	}
	
	

	return 0;
}
