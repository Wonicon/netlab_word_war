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
    char buf[1024] = { 0 };

    printf("Connected on %d\n", fd);

    // 接收到 FIN 会退出
    while (read(fd, buf, sizeof(buf))) {
		Request* q = (Request *)buf;
		if(q->type == REGISTER)  //注册报文
			handle_register(q->account.userID,q->account.passwd,fd);
		else if(q->type == LOGIN) //登录报文
			handle_login(q->account.userID,q->account.passwd,fd);
		else if(q->type == LOGOUT) //退出报文
			;
    }

    close(fd);

    printf("Close connection on %d\n", fd);

    return arg;
}

void handle_register(char *userID, char *passwd,int fd) {
	Response ack;
	if(insert_table(userID,passwd) == 0)
		ack.type = REGISTER_ACK;
	else 
		ack.type = ID_CONFICT;

	ack.single.num = 1;
	memset(ack.single.data,0,200);
	sprintf(ack.single.data,"%s%s",userID,passwd);
	//strncpy(ack.single.data,userID,);
	//strcat(ack.single.data,userID);
	//strcat(ack.single.data,passwd);

	send(fd,&ack,sizeof(Response),0);
}

void handle_login(char *userID, char *passwd, int fd) {
	Response ack;
	memset(ack.single.data,0,200);
	if(check_table(userID,passwd,ack.single.data) == NULL) {
		ack.type = LOGIN_ACK;
		send(fd,&ack,sizeof(Response),0);
	}
	else {
		ack.type = LOGIN_ERROR;
		send(fd,&ack,sizeof(Response),0);
	}
}
