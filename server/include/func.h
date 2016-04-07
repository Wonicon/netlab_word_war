#ifndef FUNC_H
#define FUNC_H

//记录所有在线玩家的用户ID的在线状态，仅在玩家登录时使用
struct online_info {
	int num;
	char data[200];
};

//每一个连接服务器的客户端都分配一个server_thread结构
struct server_thread {
	int sockfd;        //玩家登录时所在的套接字描述符
	char userID[10];   //玩家ID
	int state;         //玩家是否处于对战状态
	uint8_t attack;    //玩家出招
	uint8_t result;    //一轮的对战结果
	uint8_t HP;        //血量
};

//设定超时时间
#define TIME_OUT 2147483648

int init_table();
int insert_table(char *userID, char *passwd);
struct online_info *check_table(char *userID, char *passwd, struct online_info *q);
int alter_table(char *userId,int status);
int send_list(int socket_fd, char username[]);
int count_online(void);

void handle_register(char *userID, char *passwd,int fd);
void handle_login(char *userID, char *passwd,int fd);
void handle_logout(char *userID,int status);

void handle_askbattle(char *srcID,char *dstID,int srcfd);
void handle_yesbattle(char *srcID,char *dstID,int dstfd);
void handle_nobattle(char *srcID, char *dstID,int dstfd);
void *battle(void *argc);

Response increase_lose(const char *user);
Response increase_win(const char *user);
void get_win_lose(const char *user, uint8_t *pwin, uint8_t *plose);

#endif
