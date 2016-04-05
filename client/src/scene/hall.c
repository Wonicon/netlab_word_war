#include <ncurses.h>
#include <pthread.h>
#include "state.h"

void *thread(void *arg)
{
    WINDOW *wind = arg;
    int i = 0;
    while (1) {
        mvwprintw(wind, 0, 0, "hello %d", i++);
        pthread_mutex_lock(&mutex_refresh);
        wrefresh(wind);
        pthread_mutex_unlock(&mutex_refresh);
    }
}

/**
 * @brief 游戏大厅界面，显示在线玩家名单，排名，通知，提供选择交互功能
 */
void scene_hall(void)
{
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

    pthread_t tid;
    pthread_create(&tid, NULL, thread, win_list);
    pthread_create(&tid, NULL, thread, win_info);
    pthread_create(&tid, NULL, thread, win_help);

    char ch;
    while ((ch = getch()) != 'q') ;
}
