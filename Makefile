.PHONLY :server clean

src = $(wildcard *.c)
obj = $(patsubst %.c, %.o, $(src))

CC = gcc
CFLGAS = -g -Wall -ljpeg -lpthread
# 静态库动态库相关
# CLINK = -L ./ -Wl,-rpath=./

server : $(obj)
	$(CC) -o $@ $^ $(CFLGAS)
.c.o:
	$(CC) -c $< $(CFLGAS)


clean:
	rm *.o
	rm server
