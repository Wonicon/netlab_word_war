#include <string.h>
#include <ncurses.h>

extern void scene_login(void);
extern void scene_register(void);

void scene_welcome(void)
{
    const char TITLE[] = "Word War";
    const char LOGIN[] = "Login";
    const char REG[]   = "Register";

    curs_set(0);  // No cursor

    // Get screen size
    int lines, cols;
    getmaxyx(stdscr, lines, cols);
    
    // Calc the pos for center alignment
    int title_y = (lines - 4) / 2 - 10;
    int title_x = (cols - sizeof(TITLE) - 1) / 2;
    
    int login_y = title_y + 2;
    int login_x = (cols - sizeof(LOGIN) - 1) / 2;

    int reg_y = login_y + 1;
    int reg_x = login_x;

    attron(A_BOLD);

    int selection = 0;  // 0 for login, 1 for register
    int checked = 0;    // enter inputed ?

    while (!checked) {
        erase();

        // Draw title
        mvprintw(title_y, title_x, "%s", TITLE);
        
        // Draw login option
        if (selection == 0) {
            attron(A_UNDERLINE);
            mvprintw(login_y, login_x, "%s", LOGIN);
            attroff(A_UNDERLINE);
        }
        else {
            mvprintw(login_y, login_x, "%s", LOGIN);
        }
 
        // Draw register option
        if (selection == 1) {
            attron(A_UNDERLINE);
            mvprintw(reg_y, reg_x, "%s", REG);
            attroff(A_UNDERLINE);
        }
        else {
            mvprintw(reg_y, reg_x, "%s", REG);
        }

        // Update to screen
        refresh();

        // Update selection
        int ch = getch();

        if (ch == KEY_UP) {
            selection = 0;
        }
        else if (ch == KEY_DOWN) {
            selection = 1;
        }
        else if (ch == '\n') {
            checked = 1;
        }
    }

    erase();
    attroff(A_BOLD);

    if (selection == 0) {
        scene_login();
    }
    else {
        scene_register();
    }
}
