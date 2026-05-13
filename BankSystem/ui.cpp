#include "ui.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <conio.h>

static HANDLE hCon = INVALID_HANDLE_VALUE;

static HANDLE getCon() {
    if (hCon == INVALID_HANDLE_VALUE)
        hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    return hCon;
}

void setColor(WORD color) {
    SetConsoleTextAttribute(getCon(), color);
}
void resetColor() {
    setColor(Color::WHITE);
}
void clearScreen() {
    system("cls");
}

void printLine(char ch, int width) {
    setColor(Color::CYAN);
    for (int i = 0; i < width; ++i) putchar(ch);
    putchar('\n');
    resetColor();
}

void printHeader(const char* title) {
    clearScreen();
    setColor(Color::LCYAN);
    printLine('=');
    // 居中打印
    int len = (int)strlen(title);
    int pad = (68 - len) / 2;
    printf("%*s%s\n", pad, "", title);
    printLine('=');
    resetColor();
}

void printSubHeader(const char* title) {
    setColor(Color::LYELLOW);
    printf("\n  [ %s ]\n", title);
    setColor(Color::CYAN);
    printLine('-');
    resetColor();
}

void printSuccess(const char* msg) {
    setColor(Color::LGREEN);
    printf("  [√] %s\n", msg);
    resetColor();
}
void printError(const char* msg) {
    setColor(Color::LRED);
    printf("  [✗] %s\n", msg);
    resetColor();
}
void printWarning(const char* msg) {
    setColor(Color::LYELLOW);
    printf("  [!] %s\n", msg);
    resetColor();
}
void printInfo(const char* msg) {
    setColor(Color::LCYAN);
    printf("  [i] %s\n", msg);
    resetColor();
}

void printField(const char* label, const char* value) {
    setColor(Color::YELLOW);
    printf("  %-18s", label);
    setColor(Color::LWHITE);
    printf(": %s\n", value);
    resetColor();
}
void printFieldF(const char* label, double value, int decimals) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.*f", decimals, value);
    printField(label, buf);
}
void printFieldI(const char* label, int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    printField(label, buf);
}

void pause() {
    setColor(Color::CYAN);
    printf("\n  按任意键继续...");
    resetColor();
    _getch();
    printf("\n");
}

void getInput(const char* prompt, char* buf, int maxLen) {
    setColor(Color::LYELLOW);
    printf("  %s", prompt);
    resetColor();
    fflush(stdout);
    fgets(buf, maxLen, stdin);
    // 去除末尾换行
    int len = (int)strlen(buf);
    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r'))
        buf[--len] = '\0';
}

void getPassword(const char* prompt, char* buf, int maxLen) {
    setColor(Color::LYELLOW);
    printf("  %s", prompt);
    resetColor();
    fflush(stdout);
    int i = 0;
    int ch;
    while ((ch = _getch()) != '\r' && i < maxLen - 1) {
        if (ch == '\b') {
            if (i > 0) { --i; printf("\b \b"); }
        } else {
            buf[i++] = (char)ch;
            putchar('*');
        }
    }
    buf[i] = '\0';
    putchar('\n');
}

int getIntInput(const char* prompt, int minVal, int maxVal) {
    char buf[32];
    int val;
    while (true) {
        getInput(prompt, buf, sizeof(buf));
        if (sscanf(buf, "%d", &val) == 1 && val >= minVal && val <= maxVal)
            return val;
        setColor(Color::LRED);
        printf("  输入无效，请输入 %d-%d 之间的整数\n", minVal, maxVal);
        resetColor();
    }
}

double getDoubleInput(const char* prompt, double minVal, double maxVal) {
    char buf[32];
    double val;
    while (true) {
        getInput(prompt, buf, sizeof(buf));
        if (sscanf(buf, "%lf", &val) == 1 && val >= minVal && val <= maxVal)
            return val;
        setColor(Color::LRED);
        printf("  输入无效，请输入 %.2f-%.2f 之间的数值\n", minVal, maxVal);
        resetColor();
    }
}

int showMenu(const char* title, const char* items[], int count) {
    printSubHeader(title);
    for (int i = 0; i < count; ++i) {
        setColor(Color::LYELLOW);
        printf("  [%2d] ", i + 1);
        setColor(Color::LWHITE);
        printf("%s\n", items[i]);
    }
    setColor(Color::LYELLOW);
    printf("  [ 0] ");
    setColor(Color::LWHITE);
    printf("返回上级\n");
    printLine('-');
    resetColor();
    return getIntInput("请选择: ", 0, count);
}

bool confirm(const char* msg) {
    setColor(Color::LYELLOW);
    printf("  %s (y/n): ", msg);
    resetColor();
    fflush(stdout);
    char ch = (char)_getch();
    printf("%c\n", ch);
    return ch == 'y' || ch == 'Y';
}

void printTableHeader(const char* cols[], int widths[], int colCount) {
    setColor(Color::BG_BLUE | Color::LWHITE);
    printf("  ");
    for (int i = 0; i < colCount; ++i)
        printf("%-*s ", widths[i], cols[i]);
    printf("\n");
    resetColor();
}
void printTableRow(const char* vals[], int widths[], int colCount) {
    setColor(Color::WHITE);
    printf("  ");
    for (int i = 0; i < colCount; ++i)
        printf("%-*s ", widths[i], vals[i]);
    printf("\n");
    resetColor();
}
void printTableSep(int widths[], int colCount) {
    setColor(Color::CYAN);
    printf("  ");
    for (int i = 0; i < colCount; ++i) {
        for (int j = 0; j < widths[i]+1; ++j) putchar('-');
    }
    printf("\n");
    resetColor();
}

void showBanner() {
    clearScreen();
    setColor(Color::LCYAN);
    printf("\n");
    printf("  ╔══════════════════════════════════════════════════════════════════╗\n");
    printf("  ║                                                                  ║\n");
    printf("  ║            银  行  综  合  管  理  系  统  v1.0                  ║\n");
    printf("  ║                Bank Management System                           ║\n");
    printf("  ║                                                                  ║\n");
    setColor(Color::CYAN);
    printf("  ║   功能模块：职员管理 | 客户管理 | 银行卡管理 | 存贷款业务       ║\n");
    printf("  ║             业务查询 | 排队管理 | 网点导航   | 智能分析         ║\n");
    setColor(Color::LCYAN);
    printf("  ║                                                                  ║\n");
    printf("  ╚══════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    resetColor();
}
