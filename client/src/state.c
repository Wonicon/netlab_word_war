/**
 * @file state.c
 * @brief 存储客户端相关状态
 *
 * 状态等内容属于临界区，在此模块中定义互斥锁来进行保护。
 */

#include <pthread.h>
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
