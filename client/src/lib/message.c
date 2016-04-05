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
        return 1;
    }
}

/**
 * @brief 发送注册请求，等待服务器响应
 * @param username 用户名，限长 9 个字符
 * @param passowrd 密码，限长 9 个字符
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
