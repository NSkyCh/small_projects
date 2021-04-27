#ifndef __CAMERA_H__
#define __CAMERA_H__


int open_camera(void);
int camera_init(int cameraFd);
int camera_capturing(int cameraFd);
int read_image(int cameraFd,int acceptfd);
int build_picture(void *addr,int acceptfd);
int camera_read(int cameraFd,int acceptfd);
int ending_capturing(int cameraFd);
int close_camera(int cameraFd);
void yuyv_to_rgb(unsigned char* yuv,unsigned char* rgb);
long rgb_to_jpeg(char *rgb, char *jpeg);
int set_camera(void);


#endif