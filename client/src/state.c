/**
 * @file state.c
 * @brief 存储客户端相关状态
 *
 * 状态等内容属于临界区，在此模块中定义互斥锁来进行保护。
 */

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "state.h"

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
 * @brief 客户端状态
 */
ClientState client_state;

/**
 * @brief 当前选择的玩家，被 mutex_list 保护
 */
int selected = 0;

/**
 * @brief 用户名
 */
char userID[10] = "N/A";

pthread_mutex_t mutex_list = PTHREAD_MUTEX_INITIALIZER;

PlayerEntry *player_list = NULL;

int nr_players = 0;

/**
 * @brief 帮助窗口显示使能
 */
int help_window_en = 0;

/**
 * @brief 通知栏
 */
static struct {
    char *buf;
    size_t len;
    int level;
    pthread_mutex_t mutex;
} info = { NULL, 0, 0, PTHREAD_MUTEX_INITIALIZER };

void increase_info_level(void)
{
    pthread_mutex_lock(&info.mutex);
    info.level++;
    pthread_mutex_unlock(&info.mutex);
}

void decrease_info_level(void)
{
    pthread_mutex_lock(&info.mutex);
    info.level--;
    pthread_mutex_lock(&info.mutex);
}

void init_info(int w)
{
    info.buf = calloc((size_t)w, sizeof(char));
    info.len = (w - 1) * sizeof(char);
}

void update_info(int level, const char *fmt, ...)
{
    pthread_mutex_lock(&info.mutex);
    if (level >= info.level) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(info.buf, info.len, fmt, args);
        va_end(args);
    }
    pthread_mutex_lock(&info.mutex);
}

const char *get_info(void)
{
    return info.buf;
}
