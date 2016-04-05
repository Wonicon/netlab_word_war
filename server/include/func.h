#ifndef FUNC_H
#define FUNC_H
int insert_table(char *userID, char *passwd);
char *check_table(char *userID, char *passwd, char* data);

void handle_register(char *userID, char *passwd,int fd);
void handle_login(char *userID, char *passwd,int fd);
#endif
