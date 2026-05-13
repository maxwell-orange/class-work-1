#pragma once
#include <cstdio>
#include <cstring>
#include <windows.h>

/*=============================================================
 *  控制台颜色常量（Windows Console API）
 *=============================================================*/
namespace Color {
    static const WORD BLACK   = 0;
    static const WORD BLUE    = FOREGROUND_BLUE;
    static const WORD GREEN   = FOREGROUND_GREEN;
    static const WORD CYAN    = FOREGROUND_GREEN | FOREGROUND_BLUE;
    static const WORD RED     = FOREGROUND_RED;
    static const WORD MAGENTA = FOREGROUND_RED | FOREGROUND_BLUE;
    static const WORD YELLOW  = FOREGROUND_RED | FOREGROUND_GREEN;
    static const WORD WHITE   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    // 高亮版本
    static const WORD LBLUE   = BLUE    | FOREGROUND_INTENSITY;
    static const WORD LGREEN  = GREEN   | FOREGROUND_INTENSITY;
    static const WORD LCYAN   = CYAN    | FOREGROUND_INTENSITY;
    static const WORD LRED    = RED     | FOREGROUND_INTENSITY;
    static const WORD LMAG    = MAGENTA | FOREGROUND_INTENSITY;
    static const WORD LYELLOW = YELLOW  | FOREGROUND_INTENSITY;
    static const WORD LWHITE  = WHITE   | FOREGROUND_INTENSITY;
    // 带背景
    static const WORD BG_BLUE  = BACKGROUND_BLUE  | BACKGROUND_INTENSITY;
    static const WORD BG_GREEN = BACKGROUND_GREEN | BACKGROUND_INTENSITY;
    static const WORD BG_RED   = BACKGROUND_RED   | BACKGROUND_INTENSITY;
    static const WORD BG_CYAN  = BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
}

/*=============================================================
 *  UI 工具函数声明
 *=============================================================*/

// 设置控制台颜色
void setColor(WORD color);
// 恢复默认颜色
void resetColor();
// 清屏
void clearScreen();
// 打印固定宽度的分隔线
void printLine(char ch = '-', int width = 68);
// 打印带颜色的标题框
void printHeader(const char* title);
// 打印子标题
void printSubHeader(const char* title);
// 打印成功/错误/警告/提示信息
void printSuccess(const char* msg);
void printError(const char* msg);
void printWarning(const char* msg);
void printInfo(const char* msg);
// 打印表格行（key + value）
void printField(const char* label, const char* value);
void printFieldF(const char* label, double value, int decimals = 2);
void printFieldI(const char* label, int value);
// 暂停等待按键
void pause();
// 安全获取字符串输入（防止溢出，自动去除换行）
void getInput(const char* prompt, char* buf, int maxLen);
// 获取密码（不回显，显示 *）
void getPassword(const char* prompt, char* buf, int maxLen);
// 获取整数输入
int  getIntInput(const char* prompt, int minVal, int maxVal);
// 获取浮点输入
double getDoubleInput(const char* prompt, double minVal, double maxVal);
// 显示菜单并返回选择
int  showMenu(const char* title, const char* items[], int count);
// 显示确认对话框
bool confirm(const char* msg);
// 打印表格表头
void printTableHeader(const char* cols[], int widths[], int colCount);
// 打印表格行
void printTableRow(const char* vals[], int widths[], int colCount);
void printTableSep(int widths[], int colCount);
// 显示系统 banner
void showBanner();
