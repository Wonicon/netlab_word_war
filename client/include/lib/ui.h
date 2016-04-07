#ifndef LIB_UI_H
#define LIB_UI_H

#include <stdio.h>

// 获取输入
int get_input(int line, int col, char buf[], size_t size, int echo_en);
void bubble_sort(PlayerEntry list[], int n, int method);
const char *get_method(int t);

#endif
