#ifndef __CAMERA_H__
#define __CAMERA_H__


int open_camera(void);
extern int camera_init(int cameraFd);
int camera_capturing(int cameraFd);
extern int read_image(int cameraFd,int acceptfd);
int build_picture(void *addr,int acceptfd);
extern int camera_read(int cameraFd,int acceptfd);
int ending_capturing(int cameraFd);
int close_camera(int cameraFd);
int set_camera(void);

#endif