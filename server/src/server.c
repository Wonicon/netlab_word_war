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

void handle_register(char *userID,char *passwd,int fd);
extern int insert_table(char *UserId,char *passed);
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
       struct client* q = (struct client*)buf;
	  if(q->type == REGISTER)  //注册报文
		  handle_register(q->single.userID,q->single.passwd,fd);
	  else if(q->type == LOGIN) //登录报文
		  ;//handle_login;
	  else if(q->type == LOGOUT) //退出报文
		  ;
    }

    close(fd);

    printf("Close connection on %d\n", fd);

    return arg;
}

void handle_register(char *userID, char *passwd,int fd) {
	struct server ack;
	if(insert_table(userID,passwd) == 0)
		ack.type = REGISTER_ACK;
	else 
		ack.type = ID_CONFICT;

	ack.single.num = 1;
	memset(ack.single.data,0,200);
	strcat(ack.single.data,userID);
	strcat(ack.single.data,passwd);

	send(fd,&ack,sizeof(struct server),0);
}
