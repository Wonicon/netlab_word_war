/**
 * @file state.c
 * @brief 存储客户端相关状态
 *
 * 状态等内容属于临界区，在此模块中定义互斥锁来进行保护。
 */

#include <pthread.h>

/**
 * @brief 屏幕宽度
 */
int W = 0;

/**
 * @brief 屏幕高度
 */
int H = 0;

/**
 * @brief 客户端连接套接字
 */

int client_socket = -1;

/**
 * @brief 屏幕刷新的互斥锁
 */
pthread_mutex_t mutex_refresh = PTHREAD_MUTEX_INITIALIZER;