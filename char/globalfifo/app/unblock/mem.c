#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
int main()
{
	int fd;
	fd_set rfds,wfds;//读写文件描述符集
	/*以非阻塞方式打开设备文件*/
	fd = open("/dev/x4412-globalfifo", O_RDONLY | O_NONBLOCK);
	if (fd != - 1)
	{
		while (1)	//轮询
		{
			FD_ZERO(&rfds);//清除读文件描述符
			FD_ZERO(&wfds);//清除写文件描述符
			FD_SET(fd, &rfds);//将 rfds 加入文件描述符集
			FD_SET(fd, &wfds);//将 wfds 加入文件描述符集
			//监控读写文件描述符
			select(fd + 1, &rfds, &wfds, NULL, NULL);
			/*数据可获得*/
			if (FD_ISSET(fd, &rfds))
			{
				printf("Poll monitor:can be read\n");
			}
			/*数据可写入*/
/* 			if (FD_ISSET(fd, &wfds))
			{
				printf("Poll monitor:can be written\n");
			} */
		}
	}
	else
	{
		printf("Device open failure\n");
	}
	return 0;
}