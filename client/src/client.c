/**
 * @file     client.c
 * @brief    客户端主程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "state.h"

extern void scene_welcome(void);
extern void scene_hall(void);

/**
 * @brief  客户端程序主体
 *
 * 负责初始化 curse 环境，创建连接，并执行交互循环。
 */
int main(int argc, char *argv[])
{
    if ((argc != 3) || (argc > 1 && !strcmp(argv[1], "help"))) {
        int indent;
        printf("Usage: %n%s <server-ip> <server-port>  # connects to custom server\n", &indent, argv[0]);
        printf(      "%*s%s help                       # show this message\n", indent, "", argv[0]);
        return 0;
    }

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = inet_addr(argv[1]),
        .sin_port        = htons((uint16_t)atoi(argv[2])),
    };

    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket == -1) {
        perror(NULL);
        exit(-1);
    }

    if (connect(client_socket, (struct sockaddr *)&addr, sizeof(addr))) {
        perror(NULL);
        exit(-1);
    }

    /*
     * 初始化 curse 环境
     * 在建立连接之后做，不然中途因错误退出会导致终端混乱
     */
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);  // 获取方向键等，注意 getch 的返回值是 int
    curs_set(0);
    start_color();  // Allow us to modify color

    scene_welcome();

    scene_hall();

    echo();
    endwin();
}
