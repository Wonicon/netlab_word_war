/**
 * @file     server.c
 * @brief    服务器程序入口，负责一些初始化工作
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <proxy.h>
#include <string.h>

/*
 * 初始化服务器，获得监听套接字
 */
static int init_server(uint16_t port_no);

/*
 * 用于测试连接线程
 */
static void *echo(void *arg);
int init_table(); 

/**
 * @brief 服务器程序入口
 *
 * 负责侦听连接，并创建连接线程
 */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port-number>\n", argv[0]);
        exit(-1);
    }

    long port_no = strtol(argv[1], NULL, 10);
    if (port_no > USHRT_MAX && port_no < 0) {
        fprintf(stderr, "ERROR: port number %ld is invalid.\n", port_no);
        exit(-1);
    }

    int listen_sock = init_server((uint16_t)port_no);

    for(;;) {
        pthread_t tid;
        socklen_t sock_len;
        struct sockaddr_in addr;
        int connect_sock = accept(listen_sock, (struct sockaddr *)&addr, &sock_len);
        pthread_create(&tid, NULL, echo, (void *)(long)connect_sock);
    }
}


/**
 * @brief 初始化服务器
 * @param portno 端口号
 * @return 绑定本机地址的监听套接字
 *
 * 创建套接字，绑定本机默认 IP 地址与给定端口号。
 * 如果发生错误会直接结束程序。
 * 
 * TO do: 创建一个表，初始为空，记录当前玩家状态
 */
static int init_server(uint16_t port_no)
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Cannot open socket");
        exit(-1);
    }

    struct sockaddr_in server_address = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(port_no)
    };

    if (bind(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address))) {
        perror("Cannot bind");
        exit(-1);
    }

	init_table();
    listen(socket_fd, 5);

    return socket_fd;
}


typedef void (*request_handler)(int socket_fd, Request *msg);

void login_handler(int socket_fd, Request *msg)
{
    typeof(&(msg->account)) p = &(msg->account);
    Response response;
    printf("%s, %s\n", p->userID, p->passwd);
    if (!strcmp(p->userID, "mike") && !strcmp(p->passwd, "12345")) {
        printf("Check\n");
        response.type = LOGIN_ACK;
        write(socket_fd, &response, sizeof(response));

        PlayerEntry list[] = {
                { "Jack", 1, 1, 1 },
                { "Mike", 1, 1, 1 },
                { "Nancy", 1, 1, 1 },
                { "Nancy", 1, 1, 1 },
                { "Nancy", 1, 1, 1 },
                { "Nancy", 1, 1, 1 },
        };

        response.type = LIST_UPDATE;
        response.list.nr_entry = sizeof(list) / sizeof(list[0]) ;

        write(socket_fd, &response, sizeof(response));

        for (int i = 0; i < response.list.nr_entry; i++) {
            write(socket_fd, &list[i], sizeof(list[i]));
        }
    }
    else {
        response.type = LOGOUT_ACK;
        write(socket_fd, &response, sizeof(response));
    }
}

void ask_battle_handler(int socket_fd, Request *msg)
{
    printf("battle\n");
    Response response = { .type = YES_BATTLE };
    write(socket_fd, &response, sizeof(response));
}

request_handler handlers[255] = {
        [LOGIN] = login_handler,
        [ASK_BATTLE] = ask_battle_handler,
};

/**
 * @brief 测试连接的简单线程
 * @param arg 实际上是连接套接字
 * @return 返回 arg
 */
static void *echo(void *arg)
{
    int fd = (int)(long)arg;  // 连接套接字
    Request msg;

    printf("Connected on %d\n", fd);

    // 接收到 FIN 会退出
    while (read(fd, &msg, sizeof(msg))) {
        handlers[msg.type](fd, &msg);
    }

    close(fd);

    printf("Close connection on %d\n", fd);

    return arg;
}
