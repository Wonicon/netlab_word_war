#ifndef LIB_MESSAGE_H
#define LIB_MESSAGE_H

// 发送登陆请求，等待响应，返回是否登陆成功
int send_login_msg(char username[], char password[]);

// 发送注册请求，等待响应，返回是否注册成功
int send_register_msg(char username[], char password[]);

// 发送对战请求
void send_invitation_msg(void);

#endif
