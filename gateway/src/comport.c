#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

int SetComPort(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newtio, oldtio;

	//保存测试现有串口参数，如果串口号出错，会有相关的出错信息	
	if(tcgetattr(fd, &oldtio ) != 0)
	{
		perror("SetupSerial 1");
		return -1;
	}
	
	bzero(&newtio, sizeof(newtio));	//设置清零

	//设置字符大小
	newtio.c_cflag |= CLOCAL | CREAD;	//本地连接和接收使能
	newtio.c_cflag &= ~CSIZE;		//清除设置字符大小的位隐码
	switch(nBits)
	{
		case 7:
			newtio.c_cflag |= CS7;
			break;
		case 8:
			newtio.c_cflag |= CS8;
			break;
	}

	//设置奇偶校验位
	switch(nEvent)
	{
		case 'O':	//奇校验
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= (INPCK|ISTRIP);
			break;
		case 'E':	//偶校验
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			newtio.c_iflag |= (INPCK|ISTRIP);
			break;
		case 'N':	//无奇偶校验位
			newtio.c_cflag &= ~PARENB;
			break;
	}

	//设置波特率，输入输出都要设置
	switch(nSpeed)
	{
		case 2400:
			cfsetispeed(&newtio, B2400);
			cfsetospeed(&newtio, B2400);
			break;
		case 4800:
			cfsetispeed(&newtio, B4800);
			cfsetospeed(&newtio, B4800);
			break;
		case 9600:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
		case 19200:
			cfsetispeed(&newtio, B19200);
			cfsetospeed(&newtio, B19200);
			break;
		case 38400:
			cfsetispeed(&newtio, B38400);
			cfsetospeed(&newtio, B38400);
			break;
		case 57600:
			cfsetispeed(&newtio, B57600);
			cfsetospeed(&newtio, B57600);
			break;
		case 115200:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;
		case 460800:
			cfsetispeed(&newtio, B460800);
			cfsetospeed(&newtio, B460800);
			break;
		default:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
	}

	//设置停止位
	if(nStop == 1)
		newtio.c_cflag &= ~CSTOPB;
	else if(nStop == 2)
		newtio.c_cflag |= CSTOPB;
	
	//设置读取每个字符的等待时间，单位为100ms，没有特别要求就设为0
	newtio.c_cc[VTIME] = 0;
	//设置最少读取的字符数
	newtio.c_cc[VMIN] = 0;

	//清空之前的未接收字符
	tcflush(fd, TCIFLUSH);
	
	//激活新配置
	if((tcsetattr(fd, TCSANOW, &newtio)) != 0)
	{
		perror("com set error");
		return -1;
	}
	
	return 0;
}
