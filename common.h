#ifndef _COMMON_H
#define _COMMON_H

#include <poll.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>//操作文件夹
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>
#include <strings.h>
#include <termios.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>/* 共享内存 */
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <linux/fb.h>
#include <linux/un.h>
#include <linux/input.h>
#include <linux/videodev2.h>

#endif
