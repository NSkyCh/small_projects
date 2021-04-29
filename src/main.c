/******************************************
	#	FileName  : server.c
	#	Author	  : OYJS 
	#	QQ	      : 3014067790 
	#	Email	  : 3014067790@qq.com
	#	Created   : Fri 25 Oct 2019 11:44:42 AM CST
 ****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <linux/in.h>
#include "convert.h"
#include "camera.h"
#include "serial_com.h"

int init_server(void);
void add_epic(int epfd,int fd);

// 基本数据
typedef struct _m0_device{
	char type;
	char id;
	char len;
	char empty;
}m0_device;

// 温度
typedef struct _tem_data{
	char tem_l;
	char tem_h;
}tem_data;

// 湿度
typedef struct _hum_data{
	char hum_l;
	char hum_h;
}hum_data;

// x_y_z 以及 adc模块
typedef struct _accadc_data{
	char x;
	char y;
	char z;
	char empty;
	int adc_ch0;
	int adc_ch1;
}accadc_data;

// 光照
typedef struct _light_data{
	int light_t;
}light_data;

// 设备开关状态
typedef struct _state_data{
	char led_state;
	char fan_state;
	char buzz_state;
	char led7_state;
}state_data;

// 36 byte 数据
typedef struct _msg_data{
	m0_device devt;
	tem_data tem;
	hum_data hum;
	accadc_data accadc;
	light_data light;
	state_data state;
	int empty1;
	short crc;
	short empty2;
}msg_data;

int acceptfd[2]={-1,-1};
int cameraFd = -1,serialfd = -1;
pthread_t tid[2];

// 开关闭串口模块的标识符(0开启，1关闭)
volatile int lock_serial = 0;
volatile int lock_camera = 0;


// 串口接收模块(线程0)
void *serial(void* arg){
	msg_data *msg;
	unsigned char recvdata[36];
	char send_data[32];
	unsigned char *q;
	
	pthread_detach(pthread_self());	
	serialfd = serial_init(0);	
	if(0 > serialfd)
	{
		perror("Serial Init Error");
		exit (-1);
	}
	
	printf("%d\n",lock_serial);
	while(lock_serial == 0)
	{	
		memset(recvdata,0,sizeof(recvdata));
		q = recvdata;
		serial_Recv(serialfd, q,36, -1);
		msg = (msg_data *)recvdata;

		// 判断是否是环境指令
		if(recvdata[0] == 0xbb)
		{
			memset(send_data,0,sizeof(send_data));

			sprintf(send_data,"%d.%d_%d.%d_%d",msg->tem.tem_h, msg->tem.tem_l,\
					msg->hum.hum_h, msg->hum.hum_l, msg->light.light_t);
			printf("%s\n",send_data);
			
			write(acceptfd[0],send_data,32);
			
		}

	}
	serial_Close(serialfd);
	serialfd = -1;
	return arg;
}

// 摄像头模块(线程1)
void *camera(void* arg){
	pthread_detach(pthread_self());
	cameraFd = set_camera();
	
	printf("%d\n",lock_camera);
	while(lock_camera == 0)
	{
		//read_image(cameraFd,sockfd);
		camera_read(cameraFd,acceptfd[1]);
		//read_image(cameraFd,acceptfd[1]);
		printf("read ok!!!\n");
	}
	close_camera(cameraFd);
	cameraFd = -1;
	return arg;
}


int main(int argc, const char *argv[])
{
	
	int sockfd;
	
	struct sockaddr_in clientaddr;
	socklen_t clientlen ;

	int epfd,epct,i,re,ret,num=0;
	struct epoll_event events[20];
	struct epoll_event event;
	char buf[32],temp[32];

	unsigned char senddata[7][36]={{0x00,0x00,0x00,0x00,0x00},
								   {0xdd,0x09,0x24,0x00,0x00},
								   {0xdd,0x09,0x24,0x00,0x01},
								   {0xdd,0x09,0x24,0x00,0x04},
								   {0xdd,0x09,0x24,0x00,0x08},
								   {0xdd,0x09,0x24,0x00,0x02},
								   {0xdd,0x09,0x24,0x00,0x03}};// 1 开灯，2 关闭灯，3开风扇，4关闭风扇，5开蜂鸣器，6关闭蜂鸣器
	unsigned char *p;
	
	epfd=epoll_create(1);
	
	sockfd = init_server();
	printf("server is ok!!!\n");
	
	clientlen = sizeof(clientaddr);
		
	add_epic(epfd,sockfd);
	
		
	while(1)
	{
		epct = epoll_wait(epfd,events,20,-1);
		for(i=0;i<epct;i++)
		{
			if(events[i].data.fd == sockfd)
			{
				while(acceptfd[num] > 0)
				{
					num++;
					if(num == 0)
					{
						lock_serial = 0;			
					}
					else if(num == 1)
					{
						lock_camera = 0;
					}
					else
					{
						break;
					}
				}
				acceptfd[num] = accept(sockfd, (struct sockaddr *)&clientaddr, &clientlen);
				add_epic(epfd,acceptfd[num]);
				printf("accept ok !!!!\n");
				if(num == 0)
				{
					re = pthread_create(&tid[0], NULL,serial, NULL);
					if(re!=0)
					{
						printf("pthread_create\n");
						exit(0);
					}
				}
				else
				{

				}
				num = 0;
			}
			else if(events[i].data.fd == acceptfd[1])
			{
				// 此为发送视频数据的套接字
				memset(buf,0,sizeof(buf));
				ret = read(acceptfd[1],buf,32);
				printf("buf_ret=%d\n",ret);
				printf("buf=%s\n",buf);
				if(ret <= 0)
				{				
					if(cameraFd > 0)
					{
						// 关闭摄像头
						lock_camera = 1;
					//	close_camera(cameraFd);
					//	cameraFd = -1;
						printf("close camera!!!\n"); 						
					}
					printf("acceptfd[1] = %d is disconnected\n",acceptfd[1]);
					epoll_ctl(epfd,EPOLL_CTL_DEL,acceptfd[1],&event);
					acceptfd[1] = -1;
				}
			}
			else if(events[i].data.fd == acceptfd[0])
			{
				// 此为接收服务器信息的套接字
				memset(temp,0,sizeof(temp));
				ret = read(acceptfd[0],temp,32);
				printf("temp=%s\n",temp);
				printf("ret=%d\n",ret);

				if(ret <= 0)
				{					
					// 关闭串口
					lock_serial = 1;
				//	serial_Close(serialfd);
				//	serialfd = -1;
					
					printf("acceptfd[0] = %d is disconnected\n",acceptfd[0]);
					epoll_ctl(epfd,EPOLL_CTL_DEL,acceptfd[0],&event);
					acceptfd[0] = -1;
				}
				else if(atoi(temp) >= 1 && atoi(temp) <= 6)
				{
					// 执行指令
					p = senddata[atoi(temp)];
					serial_Send(serialfd, p, 36, -1);
				}
				else if(atoi(temp) == 7)
				{
					// 开启摄像头
					lock_camera = 0;
					re = pthread_create(&tid[1], NULL,camera, NULL);
					if(re!=0)
					{
						printf("pthread_create\n");
						exit(0);
					}

				}
				else if(atoi(temp) == 8)
				{
					// 关闭摄像头 ，此处增加锁或许效果更佳。
					lock_camera = 1;				
					printf("close camera!!!\n"); 	
				}
				/*
				switch(atoi(temp))
				{
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					{
						p = senddata[atoi(temp)];
						serial_Send(serialfd, p, 36, -1);	
						break;
					}
					case 7:
					{
						cameraFd = set_camera();
						add_epic(epfd,cameraFd);
						break;
					}
					case 8:
					{
						epoll_ctl(epfd,EPOLL_CTL_DEL,cameraFd,&event);
						close_camera(cameraFd);
						cameraFd = -1;
						printf("close camera!!!\n");	
						break;
					}
					
				}
				*/
			}

		}
	}
	return 0;	
}

// 服务器的搭建
int init_server(void)
{
	int sockfd;
	struct sockaddr_in serveraddr;
	int serverlen = sizeof(serveraddr);
	int val=1;
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		perror("sockfd");
		exit(-1);
	}

	printf("sockfd ok!\n");

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(atoi("8888"));
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));

	if(bind(sockfd,(struct sockaddr *)&serveraddr,serverlen)<0)
	{
		perror("Failed to bind");
		exit(-1);
	}
	printf("bind success\n");
	if(listen(sockfd,2)<0)
	{
		perror("Failed to listen");
		exit(-1);
	}
	printf("listen success\n");
	return sockfd;
}


void add_epic(int epfd,int fd)
{	
	struct epoll_event event;

	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
}










