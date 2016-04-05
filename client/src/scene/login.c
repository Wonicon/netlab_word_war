#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include "lib/message.h"
#include "lib/ui.h"

/**
 * @brief 登陆界面
 */
void scene_login()
{
    curs_set(2);  // 显示光标，提示用户输入

    int scr_lines, scr_cols;
    getmaxyx(stdscr, scr_lines, scr_cols);

    // 居中打印标题
    char grettings[] = "LOGIN";
    size_t len = strlen(grettings);
    int greeting_line = scr_lines / 2 - 10;
    int left_pad = (scr_cols - len) / 2;
    mvprintw(greeting_line, left_pad, "%s", grettings);

    // 打印输入项目
    int indent_name, indent_pw;
    int name_line = greeting_line + 2;
    int pw_line = name_line + 1;
    mvprintw(name_line, left_pad, "username: %n", &indent_name);
    mvprintw(pw_line, left_pad, "password: %n", &indent_pw);

    char username[10] = {};
    char password[10] = {};

    while (1) {
        // 输入用户名
        get_input(name_line, left_pad + indent_name, username, sizeof(username), 1);
        // 输入密码
        get_input(pw_line, left_pad + indent_pw, password, sizeof(password), 0);

        if (send_login_msg(username, password)) {
            break;
        }
        else {
            mvprintw(pw_line + 1, left_pad, "account info error!");
            mvprintw(name_line, left_pad + indent_name, "%*.s", strlen(username), "");
        }
    }

    curs_set(0);
}
