/******************************************
	#	FileName  : camera.c
	#	Author	  : OYJS 
	#	QQ	      : 3014067790 
	#	Email	  : 3014067790@qq.com
	#	Created   : Fri 25 Oct 2019 11:44:42 AM CST
 ****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/videodev2.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <jpeglib.h>
#include "convert.h"
#include <sys/socket.h>
#include <linux/in.h>
#include <sys/epoll.h>


/* V4L2开发中，常用的命令标志符
 *
 * VIDIOC_REQBUFS：分配内存 
 * VIDIOC_QUERYBUF：把VIDIOC_REQBUFS中分配的数据缓存转换成物理地址 
 * VIDIOC_QUERYCAP：查询驱动功能 
 * VIDIOC_ENUM_FMT：获取当前驱动支持的视频格式 
 * VIDIOC_S_FMT：设置当前驱动的频捕获格式 
 * VIDIOC_G_FMT：读取当前驱动的频捕获格式 
 * VIDIOC_TRY_FMT：验证当前驱动的显示格式 
 * VIDIOC_CROPCAP：查询驱动的修剪能力 
 * VIDIOC_S_CROP：设置视频信号的边框 
 * VIDIOC_G_CROP：读取视频信号的边框 
 * VIDIOC_QBUF：把数据从缓存中读取出来 
 * VIDIOC_DQBUF：把数据放回缓存队列 
 * VIDIOC_STREAMON：开始视频显示函数 
 * VIDIOC_STREAMOFF：结束视频显示函数 
 * VIDIOC_QUERYSTD：检查当前视频设备支持的标准，例如PAL或NTSC。
*/

/*  V4L2 流程框架 设计流程
	1.打开设备文件(/dev/video0)
	2.查看设备属性
	3.视频采集参数设置
	4.申请空间并将地址映射
	5.申请的缓存放至队列之中
	6.开始采集
	7.采集数据
	8.结束采集
	9.关闭设备
*/


typedef struct VideoBuffer {	
	void   *start;	size_t  length;
} VideoBuffer;

VideoBuffer* buffers;


/**************Version 0.6  time:2019.10.25*************/
	// 基本框架搭成，实现基本收集功能
/*************Version 0.65 time:2019.10.26**************/
	// 修复部分bug，批注加优化部分代码

/*************Version 0.8 time:2019.10.28***************/
	// 修复部分bug, 实现yuv向jpeg的转码功能
	
/*************Version 0.95 time:2019.10.28**************/
	// 修复部分bug，建立小型服务器并成功将
	// 数据传输给客户端


//开启摄像头设备文件(video0)
int open_camera(void)
{
	int cameraFd;
	cameraFd = open("/dev/video0", O_RDWR|O_NONBLOCK, 0);
	if(cameraFd < 0){
		printf("open video0 error\n");
		exit(-1);
	}
	return cameraFd;
}

// 相机初始化操作
int camera_init(int cameraFd)
{
//	v4l2_std_id std;
	int numBufs;
	int ret;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	struct v4l2_buffer buf;
	struct v4l2_capability cap;

	// 检查设备的支持标准

	struct v4l2_fmtdesc fmtdesc; fmtdesc.index=0;
	fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	printf("Support format:\n");
	while(ioctl(cameraFd, VIDIOC_ENUM_FMT, &fmtdesc) != -1)
	{
		printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description);
		fmtdesc.index++;
	}

	//查询设备属性 
	ret = ioctl(cameraFd,VIDIOC_QUERYCAP,&cap);
	if(ret < 0){
		printf("set VIDIOC_QUERYCAP is fail\n");
		exit(-1);
	}
	printf("Driver Name:%s\nCard Name:%s\nBus info:%s\nDriver Version:%u.%u.%u\n"\
			,cap.driver,cap.card,cap.bus_info,(cap.version>>16)&0XFF,\
			(cap.version>>8)&0XFF,cap.version&0XFF);


	//设置视频捕获格式
	memset(&fmt,0,sizeof(fmt));
	fmt.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 640;
	fmt.fmt.pix.height = 480;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
	
	ret = ioctl(cameraFd, VIDIOC_S_FMT, &fmt);
	if(ret < 0){
		printf("set VIDIOC_S_FMT is fail\n");
		exit(-1);
	}
	
	//分配内存
	memset(&req,0,sizeof(req));	
	req.count = 4;	
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	
	req.memory = V4L2_MEMORY_MMAP;

	ret = ioctl(cameraFd, VIDIOC_REQBUFS,&req);
	if(ret < 0){
		printf("The camera is not supported\n");
		exit(-1);
	}

	//获取并记录缓存的物理空间
	buffers = calloc(req.count, sizeof(*buffers));
	for(numBufs = 0; numBufs < req.count; numBufs++)
	{
		memset(&buf,0,sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = numBufs;
	
		//读取缓存
		ret = ioctl(cameraFd ,VIDIOC_QUERYBUF, &buf);
		if(ret < 0){
			printf("Failed to read memory\n");
			exit(-1);
		}

		//转换成相应的地址
		buffers[numBufs].length = buf.length;
		buffers[numBufs].start  = mmap(NULL,buf.length,PROT_READ | PROT_WRITE,\
			MAP_SHARED, cameraFd, buf.m.offset);
		if(buffers[numBufs].start == MAP_FAILED){
			printf("mmp is fail\n");
			exit(-1);
		}

		//放入缓存列表
		ret = ioctl(cameraFd,VIDIOC_QBUF,&buf);
		if(ret < 0){
			printf("set VIDIOC_QBUF is fail\n");
			exit(-1);
		}
	}
	
	return 0;
}
//  开始采集数据
int camera_capturing(int cameraFd)
{
		enum v4l2_buf_type type;	

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	
		if(ioctl(cameraFd, VIDIOC_STREAMON ,&type) == -1){
			printf("start capturing is fail\n");
			exit(-1);
		}
		return 0;
}

// 将数据保存为图片文件(保存)
int build_picture(void *addr,int acceptfd)
{
	//FILE *fp;
	//char picture_name[20];
	// 给rbg与jpeg分配空间
	unsigned char* rgb = (unsigned char*) malloc(640 * 480 *3);
	unsigned char* jpeg = (unsigned char*) malloc(640 * 480 * 8);
	int size;
	char rbuf[10]={0};
	int ret = 0;
	int tmp;
	/*static picture_num = 0;
	sprintf(picture_name,"video.yuv");
	sprintf(picture_name,"video%d.jpeg",picture_num);
	fp = fopen(picture_name,"w+");
	if(fp == NULL){
		printf("build picture is fail\n");
		return -1;
	}
	*/
	// yuv转换为rgb，再由rgb转化为jpeg
	convert_rgb_to_jpg_init();
	convert_yuv_to_rgb(addr, (void *)rgb, 640, 480, 24);
	size = convert_rgb_to_jpg_work((void *)rgb, (void *)jpeg, 640, 480, 24, 80);
	convert_rgb_to_jpg_exit();
	printf("size = %d\n",size);
	
	sprintf(rbuf,"%d",size);
	write(acceptfd,rbuf,10);
	
	while(ret < size)
	{
		tmp = write(acceptfd,jpeg+ret,size);//
		ret = ret + tmp;
	}
	
	usleep(10000);
	
	
	// 写入数据
	//fwrite(jpeg,size,1,fp);
	//picture_num++;

	//fclose(fp);
	free(rgb);
	free(jpeg);
	
	return 0;
}

// 读取图片
int read_image(int cameraFd,int acceptfd)
{
	struct v4l2_buffer buf;	
	int ret;
	memset(&buf,0,sizeof(buf));	
	buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;	
	buf.memory=V4L2_MEMORY_MMAP;	
	buf.index=0;
	
	// 读取缓存
	ret = ioctl(cameraFd, VIDIOC_DQBUF,&buf);
	if(ret < 0){
		printf("read_image VIDIOC_DQBUF is fail\n");
		exit(-1);
	}

	//保存图片文件 
	build_picture(buffers[buf.index].start,acceptfd);

	// 重新放入缓存队列
	ret = ioctl(cameraFd,VIDIOC_QBUF,&buf);
	if(ret < 0)
	{
		printf("VIDIOC_QBUF");
		exit(-1);
	}
	return 0;
}

// 视频采集
int camera_read(int cameraFd,int acceptfd)
{
	int t = 4;
	while(t-- > 0)
	{
		fd_set rfds;
		struct timeval tv;
		int retval;

		FD_ZERO(&rfds);
		FD_SET(cameraFd,&rfds);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		retval = select(cameraFd + 1, &rfds, NULL, NULL,&tv);
		if(retval < 0){
			printf("select error!!!\n");
			return -1;
		}
		else if(retval == 0){
			printf("timeout....\n");
		}else{
			read_image(cameraFd,acceptfd);
		}
	}
	return 0;
}

// 结束采集
int ending_capturing(int cameraFd)
{
	enum v4l2_buf_type type;
	int ret;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ret =  ioctl(cameraFd,VIDIOC_STREAMOFF,&type);
	if(ret < 0){
		printf("end capturing is fail\n");
		exit(-1);
	}
	return 0;
}

// 关闭设备
int close_camera(int cameraFd)
{
	int i;
	for(i=0;i<4;i++)
	{
		if(-1 == munmap(buffers[i].start,buffers[i].length))
		{
			printf("munmap is fail\n");
			exit(-1);
		}
	}
	free(buffers);
	close(cameraFd);

	return 0;
}

int set_camera(void)
{
	int cameraFd = open_camera();

	printf("open ok!!!\n");
	
	camera_init(cameraFd);
	printf("init ok!!!\n");

	camera_capturing(cameraFd);
	printf("capture ok!!!\n");
	
	return cameraFd;
}






