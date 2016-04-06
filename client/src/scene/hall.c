#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <proxy.h>
#include "state.h"
#include "lib/message.h"

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

        // 先检查是不是纯数据更新报文
        if (msg.type == LOGIN_ANNOUNCE) {
            pthread_mutex_lock(&mutex_list);
            player_list = realloc(player_list, (size_t)(nr_players + 1));
            strcpy(player_list[nr_players].userID, msg.account.id);
            nr_players++;
            pthread_mutex_unlock(&mutex_list);
        }
        else if (msg.type == LOGOUT_ANNOUNCE) {
            pthread_mutex_lock(&mutex_list);
            // 用最后一项覆盖要删除的项（所以不用看最后一项是谁），之后把数量 - 1，避免 realloc 开销，
            // 之后只要有新登陆的，就能回收这些空闲的空间。
            // 但是这样会破坏有序性，如果需要排序的话，还需要进一步的考量！
            for (int i = 0; i < nr_players - 1; i++) {
                if (!strcmp(player_list[i].userID, msg.account.id)) {
                    player_list[i] = player_list[nr_players - 1];  // Erase the logging-out one.
                }
            }
            nr_players--;
            pthread_mutex_unlock(&mutex_list);
        }

        switch (client_state) {
        case WAIT_REMOTE_CONFIRM:
            if (msg.type == NO_BATTLE) {
                client_state = IDLE;
            }
            else if (msg.type == YES_BATTLE) {
                client_state = BATTLING;
            }
            break;
        default: ;
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
                // 涉及 nr_players 的使用，上锁
                // TODO 能否认为这样上锁能在数据被更新之前，选定屏幕上确定的玩家？会不会出现以为选了 A 但是实际处理了 B 的情况？
                pthread_mutex_lock(&mutex_list);
                if (cmd == KEY_UP && selected > 0) selected--;
                else if (cmd == KEY_DOWN && selected < nr_players - 1) selected++;
                else if (cmd == '\n') {
                    send_invitation_msg();
                    client_state = WAIT_REMOTE_CONFIRM;
                }
                pthread_mutex_unlock(&mutex_list);
                break;
            case WAIT_LOCAL_CONFIRM: break;
            case BATTLING:
                sleep(1);
                client_state = IDLE;
                break;
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
        werase(wind);
        if (player_list) {
            for (int i = 0; i < nr_players; i++) {
                // TODO 用颜色高亮
                // TODO 并发粒度太大，选择卡顿
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
        mvwprintw(wind, 0, 0, "%s %d", (client_state == BATTLING ? "BATTLE" : "IDLE"),i++);  // 这货也不是线程安全的！
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

    send_logout_msg();

    sleep(1);
}
