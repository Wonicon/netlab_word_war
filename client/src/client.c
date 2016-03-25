/**
 * @file     client.c
 * @brief    客户端主程序
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * @brief  客户端程序主体
 *
 * 负责创建连接，并执行交互循环。
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

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        perror(NULL);
        exit(-1);
    }

    if (connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr))) {
        perror(NULL);
        exit(-1);
    }

    // 简单的 echo 交互, EOF 退出
    char buf[1024];
    while (fgets(buf, sizeof(buf), stdin)) {
        write(socket_fd, buf, strlen(buf) + 1);
    }

    close(socket_fd);
}
