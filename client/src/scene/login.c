#include <stdio.h>
#include <string.h>
#include <ncurses.h>

/**
 * @brief 登陆界面
 */
void scene_login()
{
    curs_set(2);

    int scr_lines, scr_cols;
    getmaxyx(stdscr, scr_lines, scr_cols);
    char grettings[] = "LOGIN";
    size_t len = strlen(grettings);
    int greeting_line = scr_lines / 2 - 10;
    int left_pad = (scr_cols - len) / 2;
    mvprintw(greeting_line, left_pad, "%s", grettings);

    int indent_name, indent_pw;
    int name_line = greeting_line + 2;
    int pw_line = name_line + 1;
    mvprintw(name_line, left_pad, "username: %n", &indent_name);
    mvprintw(pw_line, left_pad, "password: %n", &indent_pw);

    FILE *file = fopen("usr.log", "w");

    move(name_line, left_pad + indent_name);
    echo();
    char username[256];
    getnstr(username, sizeof(username) - 1);
    fprintf(file, "%s\n", username);

    move(pw_line, left_pad + indent_pw);
    noecho();
    char password[256];
    getnstr(password, sizeof(password) - 1);
    fprintf(file, "%s\n", password);

    curs_set(0);
}
