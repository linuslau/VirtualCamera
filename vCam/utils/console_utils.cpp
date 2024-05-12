/* Copyright(c), 2024, linuslau (liukezhao@gmail.com) */

#include "console_utils.h"   // NOLINT(build/include_subdir)

#include <windows.h>

#include <iostream>

int console_height = 0;

void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void get_console_height() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    console_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}
