/**
 * @file 定义各请求发送函数
 */

#include <string.h>

/**
 * @brief 发送登陆请求，等待服务器响应
 * @param username 用户名，限长 9 个字符
 * @param password 密码，限长 9 个字符
 * @return 用户是否存在
 *
 * TODO a fake function
 */
int send_login_msg(char username[], char password[])
{
    if (!strcmp(username, "foo") && !strcmp(password, "12345")) {
        return 1;
    }
    else {
        return 0;
    }
}

/**
 * @brief 发送注册请求，等待服务器响应
 * @param username 用户名，限长 9 个字符
 * @param passowrd 密码，限长 9 个字符
 * @return 注册是否成功
 *
 * TODO a fake function
 */
int send_register_msg(char username[], char password[])
{
    return 0;
}
