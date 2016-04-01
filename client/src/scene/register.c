#include <ncurses.h>
#include <string.h>

/**
 * @brief 注册界面
 */
void scene_register()
{
    curs_set(2);

    int lines, cols;
    getmaxyx(stdscr, lines, cols);
    char grettings[] = "REGISTER";
    size_t len = sizeof(grettings) - 1;
    int greeting_line = lines / 2 - 10;
    int left_pad = (cols - len) / 2;
    mvprintw(greeting_line, left_pad, "%s", grettings);

    int indent_name, indent_pw;
    int name_line = greeting_line + 2;
    int pw_line = name_line + 1;

    // Username
    mvprintw(name_line, left_pad, "username: %n", &indent_name);
    move(name_line, left_pad + indent_name);
    echo();
    char username[256];
    getnstr(username, sizeof(username) - 1);

    int is_confrom = 0;

    while (!is_confrom) {
        // Password
        mvprintw(pw_line, left_pad, "password: %n", &indent_pw);
        move(pw_line, left_pad + indent_pw);
        noecho();
        char password[256];
        getnstr(password, sizeof(password) - 1);

        // Confirm password
        mvprintw(pw_line + 1, left_pad, "confirm: %n", &indent_pw);
        move(pw_line + 1, left_pad + indent_pw);
        noecho();
        char confirm[256];
        getnstr(confirm, sizeof(confirm) - 1);

        if (!strcmp(password, confirm)) {
            is_confrom = 1;
        }
        else {
            mvprintw(pw_line + 2, left_pad, "passwords are not the same!");
        }
    }
    
    curs_set(0);
}
