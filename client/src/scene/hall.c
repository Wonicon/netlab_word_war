#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <proxy.h>
#include "state.h"
#include "lib/message.h"

/**
 * @brief 用于显示通知信息
 */
struct {
    char *buf;
    size_t len;
    pthread_mutex_t mutex;
    int cursor_x;
    int cursor_y;
} info_bar = {
    NULL,
    0,
    PTHREAD_MUTEX_INITIALIZER,
    -1
};

typedef struct {
    char id[10];
    uint8_t hp;
    uint8_t attack;
} Entity;

Entity rival = { };
Entity me = { };

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
            player_list = realloc(player_list, (size_t)(nr_players + sizeof(PlayerEntry)));
            strcpy(player_list[nr_players].userID, msg.account.id);
            nr_players++;
            pthread_mutex_unlock(&mutex_list);

            pthread_mutex_lock(&info_bar.mutex);
            snprintf(info_bar.buf, info_bar.len, "%s logged in", msg.account.id);
            pthread_mutex_unlock(&info_bar.mutex);
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

            pthread_mutex_lock(&info_bar.mutex);
            snprintf(info_bar.buf, info_bar.len, "%s logged out", msg.account.id);
            pthread_mutex_unlock(&info_bar.mutex);
        }

        switch (client_state) {
        // 在空闲状态下对远程的接收对战请求进行响应
        case IDLE:
            if (msg.type == ASK_BATTLE) {
                client_state = WAIT_LOCAL_CONFIRM;
                pthread_mutex_lock(&info_bar.mutex);
                snprintf(info_bar.buf, info_bar.len, "%s asks for battling (y for yes, n for no):)%n",
                         msg.account.id, &info_bar.cursor_y);
                pthread_mutex_unlock(&info_bar.mutex);
                move(H - 4, info_bar.cursor_y);  // TODO Ugly
                echo();
            }
        // 等待对战请求响应，忽视其他报文，TODO 拒绝新的对战请求
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
            else if (cmd == '\n' && selected >= 0 && selected < nr_players) {
                send_invitation_msg();
                client_state = WAIT_REMOTE_CONFIRM;
                pthread_mutex_lock(&info_bar.mutex);
                snprintf(info_bar.buf, info_bar.len, "waiting %s's response", player_list[selected].userID);
                pthread_mutex_unlock(&info_bar.mutex);
            }
            pthread_mutex_unlock(&mutex_list);
            break;
        case WAIT_LOCAL_CONFIRM:
            if (cmd == 'y') {
                client_state = BATTLING;
            }
            else if (cmd == 'n') {
                client_state = IDLE;
            }
            noecho();
            break;
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
 * @brief 绘制对战信息
 */
static const char *HP_BAR = "##########################################################################";
void draw_battle_screen(WINDOW *wind)
{
    int h, w;
    getmaxyx(wind, h, w);
    int len = 10;
    mvwprintw(wind, 0, 0, "%.*s", len, rival.id);
    mvwprintw(wind, 0, len, "%*s", rival.hp * (w - len) / 100, HP_BAR);

    mvwprintw(wind, h - 1, w - len, "%-.*s", len, me.id);
    mvwprintw(wind, h - 1, 0, "%-*.*s", me.hp * (w - len) / 100, w - len, HP_BAR);
}

/**
 * @brief 更新列表的视图线程
 *
 * TODO 有时候登陆会有乱码，应该是线程安全问题
 */
void *update_screen(void *arg)
{
    // 定义边框
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

    // 画边框
    for (int i = 0; i < sizeof(frames) / sizeof(frames[0]); i++) {
        move(frames[i].line, frames[i].col);
        switch (frames[i].method) {
            case '-': hline('-', frames[i].length); break;
            case '|': vline('|', frames[i].length); break;
            case '+': printw("+"); break;
            default: break;
        }
    }

    info_bar.buf = calloc((size_t)(W - 2), sizeof(char));
    info_bar.len = (size_t)(W - 3);

    refresh();

    WINDOW *win_list = newwin(H - 6, W - 2,     1, 1);
    WINDOW *win_info = newwin(    1, W - 2, H - 4, 1);
    WINDOW *win_help = newwin(    1, W - 2, H - 2, 1);

    wprintw(win_help, "q - quit");
    wrefresh(win_help);

    // 更新循环
    while (client_state != QUIT) {
        werase(win_list);
        if ((client_state == IDLE || client_state == WAIT_LOCAL_CONFIRM || client_state == WAIT_REMOTE_CONFIRM) && player_list) {
            pthread_mutex_lock(&mutex_list);
            for (int i = 0; i < nr_players; i++) {
                // TODO 用颜色高亮
                // TODO 并发粒度太大，选择卡顿
                mvwprintw(win_list, i, 0, "[%c] %s", (selected == i ? '*' : ' '), player_list[i].userID);
                // 这货也不是线程安全的！
            }
            pthread_mutex_unlock(&mutex_list);
        }
        else if (client_state == BATTLING) {
            draw_battle_screen(win_list);
        }
        wrefresh(win_list);

        werase(win_info);
        wprintw(win_info, "%s", info_bar.buf);
        wrefresh(win_info);
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

    strncpy(me.id, userID, sizeof(me.id) - 1);
    me.hp = 100;

    erase();

    getmaxyx(stdscr, H, W);

    struct {
        void *(*thread)(void *);
        void *arg;
        pthread_t tid;
    } threads[] = {
        { push_service, NULL },
        { user_input, NULL },
        { update_screen, NULL },
    };

    for (int i = 0; i < sizeof(threads) / sizeof(threads[i]); i++) {
        pthread_create(&threads[i].tid, NULL, threads[i].thread, threads[i].arg);
    }

    while (client_state != QUIT) ;

    send_logout_msg();

    pthread_mutex_lock(&info_bar.mutex);
    snprintf(info_bar.buf, info_bar.len, "quiting...");
    pthread_mutex_unlock(&info_bar.mutex);

    sleep(1);
}
