#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
int main()
{
	int fd;
	fd_set rfds,wfds;//��д�ļ���������
	/*�Է�������ʽ���豸�ļ�*/
	fd = open("/dev/x4412-globalfifo", O_RDONLY | O_NONBLOCK);
	if (fd != - 1)
	{
		while (1)	//��ѯ
		{
			FD_ZERO(&rfds);//������ļ�������
			FD_ZERO(&wfds);//���д�ļ�������
			FD_SET(fd, &rfds);//�� rfds �����ļ���������
			FD_SET(fd, &wfds);//�� wfds �����ļ���������
			//��ض�д�ļ�������
			select(fd + 1, &rfds, &wfds, NULL, NULL);
			/*���ݿɻ��*/
			if (FD_ISSET(fd, &rfds))
			{
				printf("Poll monitor:can be read\n");
			}
			/*���ݿ�д��*/
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