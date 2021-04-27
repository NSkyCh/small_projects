#ifndef __SERIAL_COM_H__
#define __SERIAL_COM_H__



int serial_Open(int comport);

void serial_Close(int fd);

int serial_init(int comport);

extern int serial_Recv(int fd, void *p,int data_len, int timeout);

extern int serial_Send(int fd, void *p,int data_len, int timeout);

int serial_Set(int fd, int speed, int flow_ctrl, int databits, int stopbits, int parity);

#endif
