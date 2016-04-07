#ifndef STATE_H
#define STATE_H

#include <pthread.h>
#include "proxy.h"

extern int W;  // 屏幕宽度

extern int H;  // 屏幕高度

extern int client_socket;  // 客户端连接套接字

typedef enum {
    IDLE,
    WAIT_LOCAL_CONFIRM,
    WAIT_REMOTE_CONFIRM,
    BATTLING,
    WAIT_RESULT,
    QUIT,
} ClientState;

extern int selected;

extern ClientState client_state;

extern char userID[10];

extern pthread_mutex_t mutex_list;

extern PlayerEntry *player_list;

extern int nr_players;

extern int help_window_en;

void increase_info_level(void);

void decrease_info_level(void);

void init_info(int w);

void update_info(int level, const char *fmt, ...);

const char *get_info(void);

#endif
