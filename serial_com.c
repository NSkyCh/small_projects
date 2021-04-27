#include<stdio.h>      /*标准输入输出定义*/ 
#include<stdlib.h>     /*标准函数库定义*/  
#include<unistd.h>     /*Unix 标准函数定义*/  
#include<sys/types.h>   
#include<sys/stat.h>     
#include<fcntl.h>      /*文件控制定义*/  
#include<termios.h>    /*PPSIX 终端控制定义*/  
#include<errno.h>      /*错误号定义*/  
#include<string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "serial_com.h"
extern int errno;

#include <poll.h>
#include <sys/ioctl.h>

#define FALSE  -1  
#define TRUE   0 

#define ON 1
#define OFF 0

#define eprintf(x)  printf("%s   ===file:%s     ===line:%d   ===%s \n",x,__FILE__,__LINE__,strerror(errno))

int serial_Open(int comport)  
{  
	/*分别为com1，com2， com3对应 ttyUSB0 ttyUSB1 ttyUSB2 */
	char *dev[] = {"/dev/ttyUSB0"};
	
	int fd;
    fd = open( dev[comport], O_RDWR|O_NOCTTY|O_NDELAY);  
    if (FALSE == fd)  
    {  
        perror("Can't Open Serial Port");  
        return(FALSE);
     }  
     //恢复串口为阻塞状态                                 
     if(fcntl(fd, F_SETFL, 0) < 0)  
     {  
        printf("fcntl failed!\n");  
        return(FALSE);  
     }       
     else  
     {  
        printf("fcntl=%d\n",fcntl(fd, F_SETFL,0));
     }  
      //测试是否为终端设备      
    if(0 == isatty(fd))  
    {  
        printf("standard input is not a terminal device\n");  
        return(FALSE);
	}  
	else
    {  
        printf("isatty success!\n");  
    }                
    printf("fd->open=%d\n",fd);  
    return fd;  
} 

void serial_Close(int fd)  
{  
    close(fd);  
} 

//fd串口号；speed传输波特率；databits数据位；flow_ctrl数据流；parity奇偶校验；stopbits停止位
int serial_Set(int fd,int speed,int flow_ctrl,int databits,int stopbits,int parity)  
{  
     
     int   i;  
     int   speed_arr[] = { B115200, B19200, B9600, B4800, B2400, B1200, B300};  
     int   name_arr[] = {115200,  19200,  9600,  4800,  2400,  1200,  300};  
           
    struct termios options;  
     
    /*tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options,该函数还可以测试配置是否正确，该串口是否可用等。若调用成功，函数返回值为0，若调用失败，函数返回值为1. 
    */  
    if  ( tcgetattr( fd,&options)  !=  0)  
    {  
         perror("SetupSerial 1");      
         return(FALSE);   
    }  
    
    //设置串口输入波特率和输出波特率  
    for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)  
    {  
         if  (speed == name_arr[i])  
         {               
             cfsetispeed(&options, speed_arr[i]);   
             cfsetospeed(&options, speed_arr[i]);    
         }  
   }       
     
    //修改控制模式，保证程序不会占用串口  
    options.c_cflag |= CLOCAL;  
    //修改控制模式，使得能够从串口中读取输入数据  
    options.c_cflag |= CREAD;  
    options.c_oflag &= ~(ONLCR | OCRNL);
	options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	options.c_iflag &= ~(ICRNL | INLCR);
	options.c_iflag &= ~(IXON | IXOFF | IXANY);
	
    //设置数据流控制  
    switch(flow_ctrl)  
    {  
        
       case 0 ://不使用流控制  
              options.c_cflag &= ~CRTSCTS;  
              break;     
        
       case 1 ://使用硬件流控制  
              options.c_cflag |= CRTSCTS;  
              break;  
       case 2 ://使用软件流控制  
              options.c_cflag |= IXON | IXOFF | IXANY;  
              break;  
    }  
    //设置数据位  
    //屏蔽其他标志位  
    options.c_cflag &= ~CSIZE;  
    switch (databits)  
    {    
       case 5    :  
                     options.c_cflag |= CS5;  
                     break;  
       case 6    :  
                     options.c_cflag |= CS6;  
                     break;  
       case 7    :      
                 options.c_cflag |= CS7;  
                 break;  
       case 8:      
                 options.c_cflag |= CS8;  
                 break;    
       default:     
                 fprintf(stderr,"Unsupported data size\n");  
                 return (FALSE);   
    }  
    //设置校验位  
    switch (parity)  
    {    
       case 'n':  
       case 'N': //无奇偶校验位。  
                 options.c_cflag &= ~PARENB;   
                 options.c_iflag &= ~INPCK;      
                 break;   
       case 'o':    
       case 'O'://设置为奇校验      
                 options.c_cflag |= (PARODD | PARENB);   
                 options.c_iflag |= INPCK;               
                 break;   
       case 'e':   
       case 'E'://设置为偶校验    
                 options.c_cflag |= PARENB;         
                 options.c_cflag &= ~PARODD;         
                 options.c_iflag |= INPCK;        
                 break;  
       case 's':  
       case 'S': //设置为空格   
                 options.c_cflag &= ~PARENB;  
                 options.c_cflag &= ~CSTOPB;  
                 break;   
        default:    
                 fprintf(stderr,"Unsupported parity\n");      
                 return (FALSE);   
    }   
    // 设置停止位   
    switch (stopbits)  
    {    
       case 1:     
                 options.c_cflag &= ~CSTOPB; 
				 break;   
       case 2:     
                 options.c_cflag |= CSTOPB;
				 break;  
       default:     
                       fprintf(stderr,"Unsupported stop bits\n");   
                       return (FALSE);  
    }  
     
  //修改输出模式，原始数据输出  
    options.c_oflag &= ~OPOST;  
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);//我加的  
//options.c_lflag &= ~(ISIG | ICANON);  
     
    //设置等待时间和最小接收字符  
    options.c_cc[VTIME] = 1; /* 读取一个字符等待0*(0/10)s */    
    options.c_cc[VMIN] = 1; /* 读取字符的最少个数为0 */  
     
    //如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读  
    tcflush(fd,TCIFLUSH);  
     
    //激活配置 (将修改后的termios数据设置到串口中）  
    if (tcsetattr(fd,TCSANOW,&options) != 0)    
    {  
       perror("com set error!\n");    
       return (FALSE);   
    }  
	printf("serial set success\n");
    return (TRUE);   
}  

int serial_init(int comport)  
{  
    int fd_comport = FALSE;
	//打开串口
	if((fd_comport=serial_Open(comport))<0)
	{
		perror("serial_Open");
		return FALSE;
	}
    //设置串口数据帧格式  
    if (serial_Set(fd_comport,115200,0,8,1,'N')<0)  
    {   
		perror("serial_Set");
        return FALSE;  
    }  
    return fd_comport; 
} 
 
int serial_Send(int fd, void *p,int data_len, int timeout)  
{ 
	static struct pollfd fds;
	fds.fd    = fd;
	fds.events= POLLOUT;	
	fds.revents=0;	

	/*60 second timeout*/
	int ret = poll(&fds, 1, timeout);
	if(0 >= ret)
	{
		perror("[write_serial]poll");
		return -1;	/*error or timout*/
	}

	if(fds.revents & POLLOUT)
		return write(fd, p, data_len);

	return -1;		/*unknow error*/
	return write(fd, p, data_len);
}  


int serial_Recv(int fd, void *p,int data_len, int timeout)  
{  
	static struct pollfd fds;
	fds.fd    = fd;
	fds.events= POLLIN;
	fds.revents=0;

	/*60 second timeout*/
	int ret = poll(&fds, 1, timeout);
	if(0 >= ret)
	{
		perror("[recv_serial]poll");
		return -1;	/*error or timout*/
	}

	if(fds.revents & POLLIN)
		return read(fd, p, data_len);

	printf("[TRACE][recv_serial]poll unkonw err.\r\n");
	return -1;		/*unknow error*/
	return read(fd, p, data_len);  
} 
