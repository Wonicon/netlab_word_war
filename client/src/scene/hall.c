#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "state.h"
#include "proxy.h"

pthread_mutex_t mutex_list = PTHREAD_MUTEX_INITIALIZER;

#define MAKE_PTR(p, type) typeof(type) p = type

PlayerEntry *player_list = NULL;

size_t nr_players = 0;

/**
 * @brief 一开始的齐全的在线用户列表
 *
 * TODO 每次更新整个表开销太大，应发送报文标记特定条目的更新删除
 */
void handle_init_list(Response *msg)
{
    MAKE_PTR(p, &msg->list);

    pthread_mutex_lock(&mutex_list);

    nr_players = (size_t)p->nr_entry;
    player_list = malloc(nr_players * sizeof(player_list[0]));
    for (int i = 0; i < p->nr_entry; i++) {
        read(client_socket, &player_list[i], sizeof(player_list[i]));
    }

    pthread_mutex_unlock(&mutex_list);
}

/**
 * @brief 获取服务器推送的信息，进行一部分状态转移
 *
 * 这是登陆后接收服务器报文的唯一入口。
 * 根据报文类型采取具体的处理函数，修改全局数据内容。
 * 视图部分根据更新的数据进行内容展示。
 */
void *push_service(void *arg)
{
    while (client_state != QUIT) {
        Response msg;
        read(client_socket, &msg, sizeof(msg));
        switch (msg.type) {
        case LIST_UPDATE: handle_init_list(&msg); break;
        default: break;
        }
    }

    return NULL;
}

/**
 * @brief 获取用户输入的唯一入口，进行一部分状态转移
 */
void *user_input(void *arg)
{
    while (client_state != QUIT) {
        int cmd = getch();
        if (cmd == 'q') {
            client_state = QUIT;
        }

        switch (client_state) {
            case IDLE:
                pthread_mutex_lock(&mutex_list);
                if (cmd == KEY_UP && selected > 0) selected--;
                else if (cmd == KEY_DOWN && selected < nr_players - 1) selected++;
                pthread_mutex_unlock(&mutex_list);
                break;
            case WAIT_LOCAL_CONFIRM: break;
            case WAIT_REMOTE_CONFIRM: break;
            default: ;
        }
    }
    return NULL;
}

/**
 * @brief 更新列表的视图线程
 *
 * TODO 所有视图在一个线程中更新
 */
void *update_screen(void *arg)
{
    WINDOW *wind = arg;
    while (client_state != QUIT) {
        pthread_mutex_lock(&mutex_refresh);
        pthread_mutex_lock(&mutex_list);
        if (player_list) {
            for (int i = 0; i < nr_players; i++) {
                // TODO 用颜色高亮
                mvwprintw(wind, i, 0, "[%c] %s", (selected == i ? '*' : ' '), player_list[i].userID);
                // 这货也不是线程安全的！
            }
        }
        wrefresh(wind);
        pthread_mutex_unlock(&mutex_list);
        pthread_mutex_unlock(&mutex_refresh);
    }
    return NULL;
}

void *thread(void *arg)
{
    WINDOW *wind = arg;
    int i = 0;
    while (client_state != QUIT) {
        pthread_mutex_lock(&mutex_refresh);
        mvwprintw(wind, 0, 0, "hello %d", i++);  // 这货也不是线程安全的！
        wrefresh(wind);
        pthread_mutex_unlock(&mutex_refresh);
    }
    return NULL;
}

/**
 * @brief 游戏大厅界面，显示在线玩家名单，排名，通知，提供选择交互功能
 */
void scene_hall(void)
{
    init_pair(1, COLOR_WHITE, COLOR_BLUE);

    client_state = IDLE;

    erase();

    getmaxyx(stdscr, H, W);

    struct {
        int line;
        int col;
        char method;
        int length;
    } frames[] = {
        {     0,     0, '-', W },
        {     0,     0, '|', H },
        { H - 5,     0, '-', W },
        { H - 3,     0, '-', W },
        { H - 1,     0, '-', W },
        {     0, W - 1, '|', H },
        {     0,     0, '+', 0 },
        {     0, W - 1, '+', 0 },
        { H - 5,     0, '+', 0 },
        { H - 5, W - 1, '+', 0 },
        { H - 3,     0, '+', 0 },
        { H - 3, W - 1, '+', 0 },
        { H - 1,     0, '+', 0 },
        { H - 1, W - 1, '+', 0 }
    };

    for (int i = 0; i < sizeof(frames) / sizeof(frames[0]); i++) {
        move(frames[i].line, frames[i].col);
        switch (frames[i].method) {
            case '-': hline('-', frames[i].length); break;
            case '|': vline('|', frames[i].length); break;
            case '+': printw("+"); break;
            default: break;
        }
    }

    refresh();

    WINDOW *win_list = newwin(H - 6, W - 2,     1, 1);
    WINDOW *win_info = newwin(    1, W - 2, H - 4, 1);
    WINDOW *win_help = newwin(    1, W - 2, H - 2, 1);

    struct {
        void *(*thread)(void *);
        void *arg;
        pthread_t tid;
    } threads[] = {
        { push_service, NULL },
        { user_input, NULL },
        { update_screen, win_list },
        { thread, win_info },
        { thread, win_help }
    };

    for (int i = 0; i < sizeof(threads) / sizeof(threads[i]); i++) {
        pthread_create(&threads[i].tid, NULL, threads[i].thread, threads[i].arg);
    }

    while (client_state != QUIT) ;
}
