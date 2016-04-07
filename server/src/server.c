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
#include "func.h"

/*
 * 初始化服务器，获得监听套接字
 */
static int init_server(uint16_t port_no);

/*
 * 用于测试连接线程
 */
static void *echo(void *arg);

/*
 *全局数组，记录当前在线玩家的连接套接字，和当前在该连接上登录的用户ID
 */
#define MAX_NUM_SOCKET 10
struct server_thread sockfd[MAX_NUM_SOCKET];

/*
 *根据用户ID查询用户在哪个连接上登录的,在玩家对战时需要知道用户ID在哪个套接字上登录的
 */
static int query_sockfd(char *userID) {
	int i;
	for(i = 0; i < MAX_NUM_SOCKET; i++) {
		if(strcmp(sockfd[i].userID,userID) == 0)
			return sockfd[i].sockfd;
	}

	return -1;
}

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
	for(i = 0; i < MAX_NUM_SOCKET; i++) {
		sockfd[i].sockfd = -1;
		memset(sockfd[i].userID,0,10);
	}

	//for(int i = 0; i < MAX_NUM_SOCKET; i++) sockfd[i] = -1;

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

static void send_message(int fd, const char *msg, const char *sender)
{
	Response r = { .type = MESSAGE, .msg.len = (uint8_t)strlen(msg) };
	strcpy(r.msg.srcID, sender);
	if (write(fd, &r, sizeof(r)) != sizeof(r)) perror("Send message header");   // 告知类型、来源和长度
	if (write(fd, msg, r.msg.len) != r.msg.len) perror("Send message body");  // 变长字符串
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
		if(msg.type == REGISTER)  //注册报文
			handle_register(msg.account.userID, msg.account.passwd, fd);
		else if(msg.type == LOGIN) //登录报文
			handle_login(msg.account.userID, msg.account.passwd, fd);
		else if (msg.type == LOGOUT) {
			handle_logout(msg.account.userID, fd);
			break;
		}
		else if(msg.type == ASK_BATTLE) //请求对战
			handle_askbattle(msg.battle.srcID, msg.battle.dstID, fd); //fd是邀战方的套接字
		else if(msg.type == YES_BATTLE) //答应对战
			handle_yesbattle(msg.battle.srcID, msg.battle.dstID, fd); //fd是应战方的套接字
		else if(msg.type == NO_BATTLE) //拒绝对战
			handle_nobattle(msg.battle.srcID, msg.battle.dstID, fd);  //fd是应战方的套接字
		else if(msg.type == IN_BATTLE) {//对战报文
			int pos = -1;
			for(int i = 0; i < MAX_NUM_SOCKET; i++)
				if(sockfd[i].sockfd == fd) {
					pos = i;
					break;
				}
			printf("attack\n");
			sockfd[pos].attack = msg.battle.attack;
			sockfd[pos].state = 1;
		}
		else if(msg.type == MESSAGE) {
			// 读取变长字符串
			char text[MSG_LEN] = { };
			read(fd, text, msg.msg.len);
			printf("MSG: %s (%s -> %s)\n", text, msg.msg.srcID, msg.msg.dstID);
			// 转发 TODO 上锁？
			for (int i = 0; i < MAX_NUM_SOCKET; i++) {
				if (!strcmp(sockfd[i].userID, msg.msg.dstID)) {
					send_message(sockfd[i].sockfd, text, msg.msg.srcID);
				}
			}
		}
		else if(msg.type == END_BATTLE) //某一方血量为0，结束对战
			;

		memset(&msg, 0, sizeof(msg));  // 由于使用了联合体，不清空缓冲区可能会干扰判断。
    }

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
		// 只持续维护登陆连接
		printf("%s is logging in\n", userID);
		int is_inserted = 0;
		for(int i = 0; i < MAX_NUM_SOCKET; i++) {
			printf("! %d %d\n", sockfd[i].sockfd, fd);
			if (sockfd[i].sockfd == -1) {
				sockfd[i].sockfd = fd;
				strncpy(sockfd[i].userID, userID, sizeof(sockfd[i].userID) - 1);
				is_inserted = 1;
				break;
			}
		}

		if (is_inserted) {
			//通知其他在线玩家有玩家上线
			Response announce;
			announce.type = LOGIN_ANNOUNCE,

					announce.account.num = 0x01;
			strncpy(announce.account.id, userID, sizeof(announce.account.id) - 1);
			get_win_lose(userID, &announce.account.win, &announce.account.lose);
			for(int i = 0; i < MAX_NUM_SOCKET; i++) {
				printf("%d %d\n", sockfd[i].sockfd, fd);
				if (sockfd[i].sockfd != -1 && sockfd[i].sockfd != fd) {
					send(sockfd[i].sockfd, &announce, sizeof(Response), 0);
				}
			}

			//给该玩家发送登录确认的报文
			ack.type = LOGIN_ACK;
			ack.account.num = (uint8_t)(count_online() - 1);  // 去掉自己
			printf("%s will receive %d entries\n", userID, ack.account.num);
			send(fd, &ack, sizeof(Response), 0);
			send_list(fd, userID);

			return;
		}
	}

	ack.type = LOGIN_ERROR;
	send(fd, &ack, sizeof(Response), 0);
}

void handle_logout(char *userID, int fd) {

	printf("LOGOUT: %s\n", userID);
	if(alter_table(userID, 0) == 0) {
		Response ack = { };
		ack.type = LOGOUT_ANNOUNCE;

		ack.account.num = 0x01;
		strncpy(ack.account.id, userID, sizeof(ack.account.id) - 1);

		for(int i = 0; i < MAX_NUM_SOCKET; i++) {
			if (sockfd[i].sockfd != -1 && sockfd[i].sockfd != fd)
				send(sockfd[i].sockfd, &ack, sizeof(Response), 0);
			else if (sockfd[i].sockfd == fd) {
				sockfd[i].sockfd = -1;  // Release the slot.
				memset(sockfd[i].userID,0,10);
				sockfd[i].state = 0;
			}
		}
	}
}

void handle_askbattle(char *srcID, char *dstID, int srcfd) {
	int dstfd = query_sockfd(dstID);
	Response ack;
	if(dstfd != -1) {
		ack.type = ASK_BATTLE;
		strcpy(ack.battle.srcID,srcID);
		strcpy(ack.battle.dstID,dstID);
		
		if (send(dstfd, &ack, sizeof(Response),0) != -1) {
			return;
		}

		// 进入这里说明在服务器发送了对战邀请报文后未响应前收到了目标的 FIN 以及退出。
		// 此时也要通知请求方邀请失败，但是这种情况很难构建测试。
	}
	ack.type = BATTLE_ERROR;
	printf("dst player %s is offline!\n",dstID);
	send(srcfd, &ack, sizeof(Response),0);
}

void handle_yesbattle(char *srcID, char *dstID, int dstfd) {
	int srcfd = query_sockfd(srcID);
	Response ack;
	if(srcfd != -1) {
		ack.type = YES_BATTLE;
		strcpy(ack.battle.srcID,srcID);
		strcpy(ack.battle.dstID,dstID);
		printf("yes battle: %s -> %s\n", ack.battle.srcID, ack.battle.dstID);
		send(srcfd, &ack, sizeof(Response), 0);
		
		//修改数据库，向其他所有在线玩家发送这两个玩家进入对战状态
		if(alter_table(srcID,2) == 0 && alter_table(dstID,2) == 0) {
			Response notice = { .type = BATTLE_ANNOUNCE };
			strcpy(notice.battle.srcID, srcID);
			strcpy(notice.battle.dstID, dstID);
			//通知其他在线玩家有玩家进入游戏状态
			for(int i = 0; i < MAX_NUM_SOCKET; i++)
				if(sockfd[i].sockfd != -1 && sockfd[i].sockfd != srcfd && sockfd[i].sockfd != dstfd) {
					send(sockfd[i].sockfd, &notice, sizeof(Response), 0);
				}
		}

		//创建对战线程
		struct argc {
			int srcfd;
			int dstfd;
		} a;
		
		a.srcfd = srcfd;
		a.dstfd = dstfd;

		pthread_t tid;
		pthread_create(&tid, NULL, battle, &a);
	}
	else {
		ack.type = BATTLE_ERROR;
		printf("src player %s is offline!\n", srcID);
		send(dstfd, &ack, sizeof(Response), 0);
	}
}

void handle_nobattle(char *srcID, char *dstID, int dstfd) {
	int srcfd = query_sockfd(srcID);
	Response ack;
	if(srcfd != -1) {
		ack.type = NO_BATTLE;
		strcpy(ack.battle.srcID,srcID);
		strcpy(ack.battle.dstID,dstID);

		send(srcfd, &ack, sizeof(Response),0);
	}
	else {
		ack.type = BATTLE_ERROR;
		printf("src player %s is offline!\n", srcID);
		send(srcfd, &ack, sizeof(Response), 0);
	}
}

void *battle(void *argc) {
	struct {
		int srcfd;
		int dstfd;
	} *a = argc;

	int srcfd = a->srcfd, dstfd = a->dstfd;

	//找到套接字对应的server_thread结构体
	int srcpos = -1, dstpos = -1;
	for(int i = 0; i < MAX_NUM_SOCKET; i++) {
		if (sockfd[i].sockfd == srcfd) {
			srcpos = i;
		}
		else if (sockfd[i].sockfd == dstfd) {
			dstpos = i;
		}
	}

	if (srcpos == -1 || dstpos == -1) {
		fprintf(stderr, "ERROR: connection not found");
		pthread_exit(NULL);
	}

	//进行初始化
	sockfd[srcpos].HP = INIT_HP;
	sockfd[dstpos].HP = INIT_HP;

	//当双方血量都不为0时，持续进行多轮对战

	Response r = { .type = END_BATTLE_ANNOUNCE };

	uint8_t battle_state = IN_BATTLE;
	while(battle_state == IN_BATTLE) {
		// TODO 超时处理对客户端的状态转移不利，暂不实现
		while(!(sockfd[srcpos].state >= 1 && sockfd[dstpos].state >= 1)) {
			if (sockfd[srcpos].state == 1 && sockfd[dstpos].state == 0) {
				send_message(dstfd, "Don't let your rival wait", "server");
				sockfd[srcpos].state++;
			}
			if (sockfd[srcpos].state == 0 && sockfd[dstpos].state == 1) {
				send_message(srcfd, "Don't let your rival wait", "server");
				sockfd[dstpos].state++;
			}
		}

		//如果超时
		if(0) {
			printf("timeout\n");
			//双方同时掉线，HP减1是为了防止对战线程一直在空等待
			if(sockfd[srcpos].state == 0 && sockfd[dstpos].state == 0) {
				sockfd[srcpos].result = TIE;
				sockfd[dstpos].result = TIE;
				sockfd[srcpos].HP -= 1;
				sockfd[dstpos].HP -= 1;
			}
			//dst超时
			else if(sockfd[srcpos].state == 1 && sockfd[dstpos].state == 0) {
				sockfd[srcpos].result = WIN;
				sockfd[dstpos].result = FAIL;
				sockfd[dstpos].HP -= 1;
			}
			//src超时
			else if(sockfd[srcpos].state == 0 && sockfd[dstpos].state == 1){
				sockfd[srcpos].result = FAIL;
				sockfd[dstpos].result = WIN;
				sockfd[srcpos].HP -= 1;
			}
		}
		//不超时，招式对比
		else {
			printf("battle\n");
			//出招一样
			if(sockfd[srcpos].attack == sockfd[dstpos].attack) {
				sockfd[srcpos].result = TIE;
				sockfd[dstpos].result = TIE;
			}
			//src赢（石头0x01--剪刀0x02,剪刀0x02--布0x03，布0x03--石头0x01
			else if((sockfd[srcpos].attack + 1 == sockfd[dstpos].attack) || (sockfd[srcpos].attack == sockfd[dstpos].attack + 2)) {
				sockfd[srcpos].result = WIN;
				sockfd[dstpos].result = FAIL;
				sockfd[dstpos].HP -= 1;
			}
			//dst赢
			else {
				sockfd[srcpos].result = FAIL;
				sockfd[dstpos].result = WIN;
				sockfd[srcpos].HP -= 1;
			}

		}


		if (sockfd[srcpos].HP == 0 || sockfd[dstpos].HP == 0) {
			battle_state = END_BATTLE;
			// 更新输赢，准备广播报文
			Response src, dst;
			if (sockfd[srcpos].HP == 0) {
				src = increase_lose(sockfd[srcpos].userID);
				dst = increase_win(sockfd[dstpos].userID);
			}
			else {
				dst = increase_lose(sockfd[dstpos].userID);
				src = increase_win(sockfd[srcpos].userID);
			}
			r.report.dst_win = dst.report.src_win;
			r.report.dst_lose = dst.report.src_lose;
			r.report.src_win = src.report.src_win;
			r.report.src_lose = src.report.src_lose;
		}

		//一轮处理完毕,发送结果，并为下一轮对战做准备
		Response srcack = {
			.type = battle_state,
			.battle.result = sockfd[srcpos].result,
			.battle.srcattack = sockfd[srcpos].attack,
			.battle.dstattack = sockfd[dstpos].attack,
			.battle.srcHP = sockfd[srcpos].HP,
			.battle.dstHP = sockfd[dstpos].HP
		};
		strcpy(srcack.battle.srcID, sockfd[srcpos].userID);
		strcpy(srcack.battle.dstID, sockfd[dstpos].userID);

		Response dstack = {
			.type = battle_state,
			.battle.result = sockfd[dstpos].result,
			.battle.srcattack = sockfd[dstpos].attack,
			.battle.dstattack = sockfd[srcpos].attack,
			.battle.srcHP = sockfd[dstpos].HP,
			.battle.dstHP = sockfd[srcpos].HP
		};
		strcpy(dstack.battle.srcID, sockfd[dstpos].userID);
		strcpy(dstack.battle.dstID, sockfd[srcpos].userID);

		send(sockfd[srcpos].sockfd, &srcack, sizeof(Response), 0);
		send(sockfd[dstpos].sockfd, &dstack, sizeof(Response), 0);

		sockfd[srcpos].state = 0;
		sockfd[dstpos].state = 0;
	}

	strcpy(r.report.srcID, sockfd[srcpos].userID);
	strcpy(r.report.dstID, sockfd[dstpos].userID);

	alter_table(sockfd[srcpos].userID, 1);
	alter_table(sockfd[dstpos].userID, 1);

	// TODO 对这个数组的访问是否需要上锁？
	for (int i = 0; i < MAX_NUM_SOCKET; i++) {
		int fd = sockfd[i].sockfd;
		if (fd != -1 && fd != srcfd) {
			printf("send end battle announce to %s\n", sockfd[i].userID);
			send(fd, &r, sizeof(r), 0);
		}
	}
	for (int i = 0; i < MAX_NUM_SOCKET; i++) {
		int fd = sockfd[i].sockfd;
		if (fd != -1 && fd != dstfd) {
			printf("send end battle announce to %s\n", sockfd[i].userID);
			send(fd, &r, sizeof(r), 0);
		}
	}
	return argc;
}
