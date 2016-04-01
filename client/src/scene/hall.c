#include <ncurses.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *thread(void *arg)
{
    WINDOW *wind = arg;
    int i = 0;
    while (1) {
        mvwprintw(wind, 2, 1, "hello %d", i++);
        pthread_mutex_lock(&mutex);
        wrefresh(wind);
        pthread_mutex_unlock(&mutex);
    }
}

/**
 * @brief 游戏大厅界面，显示在线玩家名单，排名，通知，提供选择交互功能
 */
void scene_hall(void)
{
    int w, h;
    getmaxyx(stdscr, h, w);

    int list_win_w = w / 2;
    int rank_win_w = w - list_win_w + 1;  // 边框重合
    int list_win_h = h - 4;
    WINDOW *list_win = newwin(list_win_h, list_win_w, 0, 0);
    WINDOW *rank_win = newwin(list_win_h, rank_win_w, 0, list_win_w - 1);  // 边框重合

    WINDOW *info_win = newwin(3, w, list_win_h - 1, 0);
    WINDOW *help_win = newwin(3, w, h - 3, 0);

    pthread_t tid;
    pthread_create(&tid, NULL, thread, rank_win);

    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    int x = 0, y = 0;
    for (;;) {
        int ch = getch();

        werase(list_win);
        werase(rank_win);
        werase(info_win);
        werase(help_win);

        wborder(list_win, '|', '|', '-', '-', '+', '+', '+', '+');
        wborder(rank_win, '|', '|', '-', '-', '+', '+', '+', '+');
        wborder(info_win, '|', '|', '-', '-', '+', '+', '+', '+');
        wborder(help_win, '|', '|', '-', '-', '+', '+', '+', '+');

        MEVENT event;
        if (ch == 'q') {
            break;
        }
        else if (ch == KEY_MOUSE && getmouse(&event) == OK) {
            mvwprintw(rank_win, 1, 1, "x %d, y %d", event.x, event.y);
        }
        switch (ch) {
            case KEY_UP:    y--; break;
            case KEY_DOWN:  y++; break;
            case KEY_LEFT:  x--; break;
            case KEY_RIGHT: x++; break;
        }
        mvwprintw(list_win, y, x, "x");
        mvwprintw(info_win, 1, 1, "x %d, y %d", x, y);

        pthread_mutex_lock(&mutex);
        wrefresh(list_win);
        //wrefresh(rank_win);
        wrefresh(info_win);
        wrefresh(help_win);
        pthread_mutex_unlock(&mutex);
    }
}
