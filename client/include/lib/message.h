#ifndef LIB_MESSAGE_H
#define LIB_MESSAGE_H

#include "proxy.h"

// 发送登陆请求，等待响应，返回是否登陆成功
int send_login_msg(char username[], char password[]);

// 发送注册请求，等待响应，返回是否注册成功
int send_register_msg(char username[], char password[]);

// 发送对战请求
void send_invitation_msg(void);

void send_logout_msg(void);

void send_battle_ack(const char src[]);

void send_attack_message(uint8_t type);

void send_no_battle(Response *ask);

#endif
