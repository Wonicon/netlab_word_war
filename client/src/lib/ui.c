/**
 * @file 与用户交互相关的模块
 */

#include <ncurses.h>

/**
 * @brief 获取用户输入
 * @param line    设置光标所在行
 * @param col     设置光标所在列
 * @param buf     接收缓冲区
 * @param size    缓冲区大小，含 '\0'
 * @param echo_en 是否回显
 * @return 返回 buf
 */
char *get_input(int line, int col, char buf[], size_t size, int echo_en)
{
    move(line, col);
    echo_en ? echo() : noecho();
    getnstr(buf, size - 1);
    buf[size - 1] = '\0';
    return buf;
}
