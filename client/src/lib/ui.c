/**
 * @file 与用户交互相关的模块
 */

#include <ncurses.h>
#include "proxy.h"

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

// 简单易懂的冒泡排序
// https://github.com/torvalds/linux/blob/b3a3a9c441e2c8f6b6760de9331023a7906a4ac6/drivers/media/common/saa7146/saa7146_hlp.c#L307
void bubble_sort(PlayerEntry list[], int n, int method)
{
    for (int top = n; top > 0; top--) {
        for (int low = 0, high = 1; high < top; low++, high++) {
            if (method == 1) {
                if (list[low].win > list[high].win) {
                    PlayerEntry temp = list[low];
                    list[low] = list[high];
                    list[high] = temp;
                }
            }
            else if (method == 2) {
                if (list[low].lose > list[high].lose) {
                    PlayerEntry temp = list[low];
                    list[low] = list[high];
                    list[high] = temp;
                }
            }
            else {
                return;
            }
        }
    }
}