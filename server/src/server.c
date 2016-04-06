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
#include <string.h>
#include <proxy.h>
#include "proxy.h"
#include "func.h"

/*
 * 初始化服务器，获得监听套接字
 */
static int init_server(uint16_t port_no);

/*
 * 用于测试连接线程
 */
static void *echo(void *arg);
int init_table(); 

/*
 *全局数组，记录当前在线玩家的连接套接字
 */
#define MAX_NUM_SOCKET 10
int sockfd[MAX_NUM_SOCKET];

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
	int i;
	for(i = 0; i < MAX_NUM_SOCKET; i++)
		sockfd[i] = -1;

    for(;;) {
        pthread_t tid;
        socklen_t sock_len;
        struct sockaddr_in addr;
        int connect_sock = accept(listen_sock, (struct sockaddr *)&addr, &sock_len);

		int i;
		for(i = 0; i < MAX_NUM_SOCKET; i++)
			if(sockfd[i] == -1) {
				sockfd[i] = connect_sock;
				break;
			}

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
		//Request* q = (Request *)buf;
		if(msg.type == REGISTER)  //注册报文
			handle_register(msg.account.userID, msg.account.passwd, fd);
		else if(msg.type == LOGIN) //登录报文
			handle_login(msg.account.userID, msg.account.passwd, fd);
    }

	handle_logout(msg.account.userID,fd);
    close(fd);

    printf("Close connection on %d\n", fd);

    return arg;
}

void handle_register(char *userID, char *password,int fd) {
	Response ack = {
			.type = (insert_table(userID, password) == 0) ? REGISTER_ACK : ID_CONFLICT,
	};
	send(fd, &ack, sizeof(ack), 0);
}

void handle_login(char *userID, char *passwd, int fd) {
	Response ack = { };
	struct online_info q = { };

	if(check_table(userID, passwd, &q) != NULL) {
		//通知其他在线玩家有玩家上线
		Response announce;
		announce.type = LOGIN_ANNOUNCE,
		announce.account.num = 0x01;
		strncpy(announce.account.id, userID, sizeof(announce.account.id) - 1);

		for(int i = 0; i < MAX_NUM_SOCKET; i++) {
			if (sockfd[i] != -1 && sockfd[i] != fd) {
				send(sockfd[i], &announce, sizeof(Response), 0);
			}
		}

		//给该玩家发送登录确认的报文
		ack.type = LOGIN_ACK;
		ack.account.num = count_online() - 1;  // 去掉自己
		printf("%s will receive %d entries\n", userID, ack.account.num);
		send(fd, &ack, sizeof(Response), 0);
		send_list(fd, userID);
	}
	else {
		ack.type = LOGIN_ERROR;
		send(fd, &ack, sizeof(Response), 0);
	}
}

void handle_logout(char *userID,int fd) {

	printf("LOGOUT: %s\n", userID);
	if(alter_table(userID, 0) == 0) {
		Response ack = { };
		ack.type = LOGOUT_ANNOUNCE;
		ack.account.num = 0x01;
		strncpy(ack.account.id, userID, sizeof(ack.account.id) - 1);

		for(int i = 0; i < MAX_NUM_SOCKET; i++)
			if(sockfd[i] != -1 && sockfd[i] != fd)
				send(sockfd[i],&ack,sizeof(Response),0);
	}
}
