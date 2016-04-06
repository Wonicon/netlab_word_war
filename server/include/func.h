#ifndef FUNC_H
#define FUNC_H

//记录所有在线玩家的用户ID的在线状态，仅在玩家登录时使用
struct online_info {
	int num;
	char data[200];
};

int insert_table(char *userID, char *passwd);
struct online_info *check_table(char *userID, char *passwd, struct online_info *q);
int alter_table(char *userId,int status);
int send_list(int socket_fd, char username[]);
int count_online(void);

void handle_register(char *userID, char *passwd,int fd);
void handle_login(char *userID, char *passwd,int fd);
void handle_logout(char *userID,int status);
#endif
