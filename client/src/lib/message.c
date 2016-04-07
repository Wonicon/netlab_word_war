/**
 * @file 定义各请求发送函数
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <proxy.h>
#include "state.h"
#include <stdio.h>

/**
 * @brief 一开始的齐全的在线用户列表
 *
 * TODO 每次更新整个表开销太大，应发送报文标记特定条目的更新删除
 */
void handle_init_list(int n)
{
    pthread_mutex_lock(&mutex_list);
    nr_players = n;
    player_list = malloc(nr_players * sizeof(player_list[0]));
    for (int i = 0; i < n; i++) {
        read(client_socket, &player_list[i], sizeof(player_list[i]));
    }
    pthread_mutex_unlock(&mutex_list);
}

/**
 * @brief 发送登陆请求，等待服务器响应
 * @param username 用户名，限长 9 个字符
 * @param password 密码，限长 9 个字符
 * @return 用户是否存在
 */
int send_login_msg(char username[], char password[])
{
    // Construct login request
    Request request = { .type = LOGIN };
    strcpy(request.account.userID, username);
    strcpy(request.account.passwd, password);

    // Send login request
    write(client_socket, &request, sizeof(request));

    // Receive ack
    Response response;
    if (read(client_socket, &response, sizeof(response)) != sizeof(response)) {
        return 0;
    }
    else if (response.type != LOGIN_ACK) {
        return 0;
    }
    else {
        strncpy(userID, username, sizeof(userID) - 1);  // Update client record.
        handle_init_list(response.account.num);
        return 1;
    }
}

/**
 * @brief 登出请求
 */
void send_logout_msg(void)
{
    // Construct logout request
    Request request = { .type = LOGOUT };
    strcpy(request.account.userID, userID);

    // Send logout request
    write(client_socket, &request, sizeof(request));
}

/**
 * @brief 发送注册请求，等待服务器响应
 * @param username 用户名，限长 9 个字符
 * @param password 密码，限长 9 个字符
 * @return 注册是否成功
 */
int send_register_msg(char username[], char password[])
{
    // Construct register request
    Request request = { .type = REGISTER };
    strcpy(request.account.userID, username);
    strcpy(request.account.passwd, password);

    // Send register request
    write(client_socket, &request, sizeof(request));

    // Receive ack
    Response response;
    if (read(client_socket, &response, sizeof(response)) != sizeof(response)) {
        return 0;
    }
    else if (response.type != REGISTER_ACK) {
        return 0;
    }
    else {
        return 1;
    }
}

/**
 * @brief 发送对战请求
 *
 * 此函数处于用户列表上锁状态
 */
 void send_invitation_msg(void)
 {
     extern PlayerEntry *player_list;

     // Construct ask request
     Request msg = { .type = ASK_BATTLE };
     MAKE_PTR(p, msg.battle);
     strcpy(p->srcID, userID);
     strcpy(p->dstID, player_list[selected].userID);

     // Send request
     write(client_socket, &msg, sizeof(msg));
 }

/**
 * @brief 发送确认对战要求的报文
 */
void send_battle_ack(const char src[])
{
    // Construct yes
    Request msg = { .type = YES_BATTLE };
    strncpy(msg.battle.dstID, userID, sizeof(msg.battle.dstID));
    strncpy(msg.battle.srcID, src, sizeof(msg.battle.srcID));

    // Send yes
    write(client_socket, &msg, sizeof(msg));
}

/**
 * @brief 发送出招报文
 * @param type 出招类型 石头 0x01 剪刀 0x02 布 0x03
 */
void send_attack_message(uint8_t type)
{
    Request msg = {
            .type = IN_BATTLE,
            .battle.attack = type
    };
    write(client_socket, &msg, sizeof(msg));
}

void send_no_battle(Response *ask)
{
    // 发送拒绝对战请求的报文，会发送给 src！
    Request nobattle = { .type = NO_BATTLE };
    strcpy(nobattle.battle.srcID, ask->battle.srcID);
    strcpy(nobattle.battle.dstID, ask->battle.dstID);
    write(client_socket, &nobattle, sizeof(nobattle));
}
