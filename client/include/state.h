#ifndef STATE_H
#define STATE_H

#include <pthread.h>

extern int W;  // 屏幕宽度

extern int H;  // 屏幕高度

extern int client_socket;  // 客户端连接套接字

extern pthread_mutex_t mutex_refresh;  // 屏幕刷新的互斥锁

#endif
