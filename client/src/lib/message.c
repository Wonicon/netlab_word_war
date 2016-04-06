/**
 * @file 定义各请求发送函数
 */

#include <string.h>
#include <unistd.h>
#include "proxy.h"
#include "state.h"

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
    write(client_state, &request, sizeof(request));
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
