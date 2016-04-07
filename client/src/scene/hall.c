#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <proxy.h>
#include "state.h"
#include "lib/message.h"
#include "lib/ui.h"

static int sort_method = 0;  // 决定排序方法 0 不排序，1 win 升序，2 lose 升序

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
            player_list = realloc(player_list, (nr_players + 1) * sizeof(PlayerEntry));
            strcpy(player_list[nr_players].userID, msg.account.id);
            player_list[nr_players].win = msg.account.win;
            player_list[nr_players].lose = msg.account.lose;
            player_list[nr_players].state = 0;
            nr_players++;
            pthread_mutex_unlock(&mutex_list);

            update_info(0, "%s logged in", msg.account.id);
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

            update_info(0, "%s logged out", msg.account.id);
        }
        else if (msg.type == BATTLE_ANNOUNCE) {
            pthread_mutex_lock(&mutex_list);
            // 使用 response.battle 根据两个 id 找到交战双方
            for (int i = 0; i < nr_players; i++) {
                if (!strcmp(player_list[i].userID, msg.battle.srcID)) {
                    player_list[i].state = ENT_STATE_BUSY;
                    break;
                }
            }
            for (int i = 0; i < nr_players; i++) {
                if (!strcmp(player_list[i].userID, msg.battle.dstID)) {
                    player_list[i].state = ENT_STATE_BUSY;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex_list);
        }
        else if (msg.type == END_BATTLE_ANNOUNCE) {
            pthread_mutex_lock(&mutex_list);
            // 使用 response.report 根据两个 id 找到交战双方
            for (int i = 0; i < nr_players; i++) {
                if (!strcmp(player_list[i].userID, msg.report.srcID)) {
                    player_list[i].state = 0;
                    player_list[i].win = msg.report.src_win;
                    player_list[i].lose = msg.report.src_lose;
                    break;
                }
            }
            for (int i = 0; i < nr_players; i++) {
                if (!strcmp(player_list[i].userID, msg.report.dstID)) {
                    player_list[i].state = 0;
                    player_list[i].win = msg.report.dst_win;
                    player_list[i].lose = msg.report.dst_lose;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex_list);
        }
        else if (msg.type == MESSAGE) {
            char text[MSG_LEN] = { };
            read(client_socket, text, msg.msg.len);
            update_info(3, "'%s' from %s", text, msg.msg.srcID);
        }

        switch (client_state) {
        // 在空闲状态下对远程的接收对战请求进行响应
        case IDLE:
            if (msg.type == ASK_BATTLE) {
                client_state = WAIT_LOCAL_CONFIRM;
                strncpy(rival.id, msg.battle.srcID, sizeof(rival.id));
                pthread_mutex_lock(&mutex_list);
                for (int i = 0;  i< nr_players; i++) {
                    if (!strcmp(player_list[i].userID, rival.id)) {
                        player_list[i].state = ENT_STATE_ASK;
                        break;
                    }
                }
                pthread_mutex_unlock(&mutex_list);
                //snprintf(info_bar.buf, info_bar.len, "%s asks for battling (y for yes, n for no)", msg.battle.srcID);
            }
            break;
        // 等待对战请求响应，忽视其他报文，TODO 拒绝新的对战请求
        case WAIT_REMOTE_CONFIRM:
            if (msg.type == NO_BATTLE || msg.type == BATTLE_ERROR) {
                update_info(0, "invitation for %s failed", msg.battle.dstID);
                client_state = IDLE;
            }
            else if (msg.type == YES_BATTLE) {
                // 状态转移
                me.hp = 5;
                rival.hp = 5;
                increase_info_level();
                client_state = BATTLING;
                // 缓存对手信息
                strncpy(rival.id, msg.battle.dstID, sizeof(rival.id));
                // Update info bar
                update_info(1, "battling with %s, type 1, 2, 3", rival.id);
            }
            else if (msg.type == ASK_BATTLE) {
                send_no_battle(&msg);
            }
            break;
        case WAIT_LOCAL_CONFIRM:
        case BATTLING:
            if (msg.type == ASK_BATTLE) {
                send_no_battle(&msg);
            }
            break;
        case WAIT_RESULT:
            if (msg.type == ASK_BATTLE) {
                send_no_battle(&msg);
            }
            // 根据服务器的裁决采取行动
            if (msg.type == IN_BATTLE || msg.type == END_BATTLE) {
                me.hp = msg.battle.srcHP;
                rival.hp = msg.battle.dstHP;
                // 打印战果
                if (msg.battle.result == TIE) {
                    update_info(1, "%s using %s ties %s using %s\n",
                                msg.battle.srcID, get_method(msg.battle.srcattack),
                                msg.battle.dstID, get_method(msg.battle.dstattack));
                }
                else if (msg.battle.result == WIN) {
                    update_info(1, "%s using %s wins %s using %s\n",
                                msg.battle.srcID, get_method(msg.battle.srcattack),
                                msg.battle.dstID, get_method(msg.battle.dstattack));
                }
                else if (msg.battle.result == FAIL) {
                    update_info(1, "%s using %s wins %s using %s\n",
                             msg.battle.dstID, get_method(msg.battle.dstattack),
                             msg.battle.srcID, get_method(msg.battle.srcattack));
                }
                else {
                    update_info(1, "WTF");
                }
                client_state = BATTLING;
            }

            if (msg.type == END_BATTLE) {
                sleep(1);
                if (msg.battle.result == WIN) {
                    update_info(1, "you have won %s", msg.battle.dstID);
                }
                else {
                    update_info(1, "you have been beaten by %s", msg.battle.dstID);
                }
                decrease_info_level();
                client_state = IDLE;
            }
            break;
        default: ;
        }
    }

    pthread_exit(NULL);
}

/**
 * @brief 获取用户输入的唯一入口，进行一部分状态转移
 */
void *user_input(void *arg)
{
    while (client_state != QUIT) {
        int cmd = getch();

        // 对简单命令进行处理
        if (cmd == 'q') {
            client_state = QUIT;
        }
        else if (cmd == 'h') {
            help_window_en = !help_window_en;  // flip-flop
        }
        else if (cmd == 'w') {
            sort_method = (sort_method == 1) ? 0 : 1;  // sort by win, flip-flop
        }
        else if (cmd == 'l') {
            sort_method = (sort_method == 2) ? 0 : 2;  // sort by lose, flip-flop
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
                update_info(1, "waiting %s's response", player_list[selected].userID);
            }
            pthread_mutex_unlock(&mutex_list);
            break;
        case WAIT_LOCAL_CONFIRM:
            if (cmd == 'y') {
                // 用于显示的初始血量
                me.hp = 5;
                rival.hp = 5;
                // 通知对方
                send_battle_ack(rival.id);
                // 修改表单
                pthread_mutex_lock(&mutex_list);
                for (int i = 0; i < nr_players; i++) {
                    // 编号可能改变，所以还是遍历搜索
                    if (!strcmp(player_list[i].userID, rival.id)) {
                        player_list[i].state = 0;  // 恢复到初始状态，对战双方不需要显示正在对战
                    }
                }
                pthread_mutex_unlock(&mutex_list);
                // 更新 info bar
                update_info(1, "battling with %s, type 1, 2, 3", rival.id);
                // 状态转移
                increase_info_level();
                client_state = BATTLING;
            }
            else if (cmd == 'n') {
                // 修改表单
                pthread_mutex_lock(&mutex_list);
                for (int i = 0; i < nr_players; i++) {
                    // 编号可能改变，所以还是遍历搜索
                    if (!strcmp(player_list[i].userID, rival.id)) {
                        player_list[i].state = 0;  // 恢复到初始状态
                    }
                }
                pthread_mutex_unlock(&mutex_list);
                client_state = IDLE;
            }
            break;
        // 对战状态，处理用户输入，并转入等待服务器的裁决
        case BATTLING: {
            uint8_t attack_type;
            switch (cmd) {
            case '1': attack_type = STONE; break;
            case '2': attack_type = SCISSOR; break;
            case '3': attack_type = PAPER; break;
            // 消息系统
            case ':': {
                update_info(3, ":");
                char msg[MSG_LEN] = { };
                int i = 0;
                while (i < MSG_LEN && (msg[i] = (char)getch()) != '\n') {
                    update_info(3, ":%s", msg);
                    i++;
                }
                msg[i] = '\0';  // 不要 '\n'，注意没有 i++
                Request r = { .type = MESSAGE, .msg.len = (uint8_t)strlen(msg) };
                strcpy(r.msg.dstID, rival.id);
                strcpy(r.msg.srcID, userID);
                write(client_socket, &r, sizeof(r));
                write(client_socket, msg, r.msg.len);
                update_info(1, "sent");
            }
            default: attack_type = 0;
            }
            if (attack_type != 0) {
                update_info(1, "attack on %s with %s, waiting response...", rival.id, get_method(attack_type));
                send_attack_message(attack_type);
                client_state = WAIT_RESULT;
            }
            break;
        }
        default: ;
        }
    }

    pthread_exit(NULL);
}

/**
 * @brief 绘制对战信息
 */
void draw_battle_screen(WINDOW *wind)
{
    int h, w;
    getmaxyx(wind, h, w);
    int len = 10;
    mvwprintw(wind, 0, 0, "%-*.*s", len, len, rival.id);  // left-aligned
    for (int i = 0;  i < rival.hp; i++) mvwprintw(wind, 0, w - rival.hp + i, "#");

    mvwprintw(wind, h - 1, w - len, "%*.*s", len, len, me.id);
    for (int i = 0;  i < me.hp; i++) mvwprintw(wind, h - 1, i, "#");
}


/**
 * @brief 输出帮助信息
 * @param file 帮助文件的文件名
 * @param wind 窗口
 */
static void draw_help(const char *filename, WINDOW *window)
{
    werase(window);
    FILE *help_text = fopen(filename, "r");  // 帮助文本
    if (help_text != NULL) {
        char line_buf[1024];
        while (fgets(line_buf, sizeof(line_buf), help_text)) {
            wprintw(window, "%s", line_buf);
        }
        fclose(help_text);
    }
    else {
        wprintw(window, "ERROR: help file '%s' cannot find", filename);
    }
    wrefresh(window);
}


/**
 * @brief 游戏大厅界面，显示在线玩家名单，排名，通知，提供选择交互功能
 */
void scene_hall(void)
{
    /*
     * 绘制边框
     */

    erase();

    getmaxyx(stdscr, H, W);

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

    init_info(W - 2);

    refresh();

    /*
     * 初始化客户端状态，启动状态转移线程。
     */

    client_state = IDLE;
    strncpy(me.id, userID, sizeof(me.id) - 1);

    struct {
        void *(*thread)(void *);
        void *arg;
        pthread_t tid;
    } threads[] = {
        { push_service, NULL },
        { user_input, NULL },
    };

    for (int i = 0; i < sizeof(threads) / sizeof(threads[i]); i++) {
        pthread_create(&threads[i].tid, NULL, threads[i].thread, threads[i].arg);
    }

    /*
     * 本函数变成绘图线程
     */

    WINDOW *win_list = newwin(H - 6, W - 2,     1, 1);
    WINDOW *win_info = newwin(    1, W - 2, H - 4, 1);
    WINDOW *win_help = newwin(    1, W - 2, H - 2, 1);

    wprintw(win_help, "logged in as %s | q: quit | h: help ", userID);
    wrefresh(win_help);

    // 更新循环
    while (client_state != QUIT) {
        if (help_window_en) {
            draw_help("help.txt", win_list);
            continue;
        }
        werase(win_list);
        if ((client_state == IDLE || client_state == WAIT_LOCAL_CONFIRM || client_state == WAIT_REMOTE_CONFIRM) && player_list) {
            pthread_mutex_lock(&mutex_list);
            bubble_sort(player_list, nr_players, sort_method);
            for (int i = 0; i < nr_players; i++) {
                // TODO 用颜色高亮
                // TODO 并发粒度太大，选择卡顿

                // 标志当前选中的 entry
                char ch = (char)(selected == i ? '*' : ' ');

                // 打印 entry 名，并根据状态进一步输出信息
                if (player_list[i].state == ENT_STATE_ASK) {
                    mvwprintw(win_list, i, 0, "[%c] %s asking for battle", ch, player_list[i].userID);
                }
                else if (player_list[i].state == ENT_STATE_BUSY) {
                    mvwprintw(win_list, i, 0, "[%c] %s is battling", ch, player_list[i].userID);
                }
                else {
                    mvwprintw(win_list, i, 0, "[%c] %s: win %d, lose %d",
                              ch, player_list[i].userID, player_list[i].win , player_list[i].lose);
                }
                // 这货也不是线程安全的！
            }
            pthread_mutex_unlock(&mutex_list);
        }
        else if (client_state == BATTLING || client_state == WAIT_RESULT) {
            draw_battle_screen(win_list);
        }
        wrefresh(win_list);

        werase(win_info);
        wprintw(win_info, "%s", get_info());
        wrefresh(win_info);
    }

    send_logout_msg();

    close(client_socket);

    update_info(3, "quiting...");

    for (int i = 0; i < sizeof(threads) / sizeof(threads[i]); i++) {
        pthread_join(threads[i].tid, NULL);
    }
}
