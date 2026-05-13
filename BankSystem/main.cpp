#include "types.h"
#include "ui.h"
#include "fileio.h"
#include "employee.h"
#include "customer.h"
#include "bankcard.h"
#include "transaction.h"
#include "bankqueue.h"
#include "branchgraph.h"
#include "smartmgr.h"
#include <cstring>
#include <cstdlib>

/*=============================================================
 *  全局会话（所有模块通过 extern 访问）
 *=============================================================*/
Session g_session;

/*=============================================================
 *  前向声明
 *=============================================================*/
void showLoginMenu(EmployeeManager&, CustomerManager&);
void adminMainMenu(EmployeeManager&, CustomerManager&, BankCardManager&,
                   TransactionManager&, BankQueueManager&,
                   BranchGraphManager&, SmartManager&);
void employeeMainMenu(EmployeeManager&, CustomerManager&, BankCardManager&,
                      TransactionManager&, BankQueueManager&,
                      BranchGraphManager&, SmartManager&);
void customerMainMenu(CustomerManager&, BankCardManager&,
                      TransactionManager&, BankQueueManager&,
                      BranchGraphManager&);

/*=============================================================
 *  登录界面
 *=============================================================*/
void showLoginMenu(EmployeeManager& empMgr, CustomerManager& custMgr) {
    const char* opts[] = {"管理员/职员登录", "客户登录"};
    while (true) {
        showBanner();
        int choice = showMenu("请选择登录方式", opts, 2);
        if (choice == 0) return;  // 退出

        char id[MAX_ID], pwd[MAX_PASS];
        if (choice == 1) {
            // 职员 / 管理员
            printSubHeader("职员登录");
            getInput("职员编号: ", id, MAX_ID);
            getPassword("登录密码: ", pwd, MAX_PASS);
            Employee* emp = empMgr.login(id, pwd);
            if (!emp) { printError("账号或密码错误！"); pause(); continue; }
            g_session.role = emp->isAdmin ? Role::ADMIN : Role::EMPLOYEE;
            strncpy(g_session.userId,   emp->id,   MAX_ID   - 1);
            strncpy(g_session.userName, emp->name, MAX_NAME - 1);
            char buf[64];
            snprintf(buf, sizeof(buf), "欢迎，%s (%s)！",
                     emp->name, emp->isAdmin ? "管理员" : "职员");
            printSuccess(buf);
            pause();
            return;
        } else {
            // 客户
            printSubHeader("客户登录");
            getInput("客户编号: ", id, MAX_ID);
            getPassword("登录密码: ", pwd, MAX_PASS);
            Customer* cust = custMgr.loginCustomer(id, pwd);
            if (!cust) { printError("账号或密码错误！"); pause(); continue; }
            g_session.role = Role::CUSTOMER;
            strncpy(g_session.userId,   cust->id,   MAX_ID   - 1);
            strncpy(g_session.userName, cust->name, MAX_NAME - 1);
            char buf[64];
            snprintf(buf, sizeof(buf), "欢迎，%s (%s客户)！",
                     cust->name, custTypeName(cust->type));
            printSuccess(buf);
            pause();
            return;
        }
    }
}

/*=============================================================
 *  管理员主菜单
 *=============================================================*/
void adminMainMenu(EmployeeManager& empMgr, CustomerManager& custMgr,
                   BankCardManager& cardMgr, TransactionManager& txnMgr,
                   BankQueueManager& queueMgr, BranchGraphManager& branchMgr,
                   SmartManager& smartMgr) {
    const char* items[] = {
        "职员管理",       // 1
        "客户账户管理",   // 2
        "银行卡管理",     // 3
        "存贷款业务",     // 4
        "业务查询",       // 5
        "排队管理",       // 6
        "网点查询导航",   // 7
        "智能管理",       // 8
        "注销登录"        // 9
    };
    while (true) {
        { char hdr[80]; snprintf(hdr, sizeof(hdr), "管理员主菜单 - " SYSTEM_NAME); printHeader(hdr); }
        char subtitle[80];
        snprintf(subtitle, sizeof(subtitle), "当前用户: %s [管理员]", g_session.userName);
        printInfo(subtitle);
        int c = showMenu("功能模块", items, 9);
        if      (c == 1) empMgr.menuAdmin();
        else if (c == 2) custMgr.menuEmployee(&cardMgr);
        else if (c == 3) cardMgr.menuEmployee();
        else if (c == 4) txnMgr.menuEmployee();
        else if (c == 5) txnMgr.menuEmployee();  // 查询已集成在业务菜单中
        else if (c == 6) queueMgr.menuAdmin();
        else if (c == 7) branchMgr.menuAdmin();
        else if (c == 8) smartMgr.menuAdmin();
        else if (c == 9 || c == 0) {
            g_session = Session();
            printInfo("已注销登录");
            return;
        }
    }
}

/*=============================================================
 *  职员主菜单
 *=============================================================*/
void employeeMainMenu(EmployeeManager& empMgr, CustomerManager& custMgr,
                      BankCardManager& cardMgr, TransactionManager& txnMgr,
                      BankQueueManager& queueMgr, BranchGraphManager& branchMgr,
                      SmartManager& smartMgr) {
    const char* items[] = {
        "客户账户管理",   // 1
        "银行卡管理",     // 2
        "存贷款业务",     // 3
        "业务查询",       // 4（嵌套在存贷款中）
        "排队管理",       // 5
        "网点查询导航",   // 6
        "智能辅助",       // 7
        "个人信息管理",   // 8
        "注销登录"        // 9
    };
    while (true) {
        { char hdr[80]; snprintf(hdr, sizeof(hdr), "职员主菜单 - " SYSTEM_NAME); printHeader(hdr); }
        char subtitle[80];
        snprintf(subtitle, sizeof(subtitle), "当前用户: %s [职员]", g_session.userName);
        printInfo(subtitle);
        int c = showMenu("功能模块", items, 9);
        if      (c == 1) custMgr.menuEmployee(&cardMgr);
        else if (c == 2) cardMgr.menuEmployee();
        else if (c == 3) txnMgr.menuEmployee();
        else if (c == 4) txnMgr.menuEmployee();
        else if (c == 5) queueMgr.menuEmployee(&empMgr);
        else if (c == 6) branchMgr.menuQuery();
        else if (c == 7) smartMgr.menuEmployee();
        else if (c == 8) empMgr.menuEmployee();
        else if (c == 9 || c == 0) {
            g_session = Session();
            printInfo("已注销登录");
            return;
        }
    }
}

/*=============================================================
 *  客户主菜单
 *=============================================================*/
void customerMainMenu(CustomerManager& custMgr, BankCardManager& cardMgr,
                      TransactionManager& txnMgr, BankQueueManager& queueMgr,
                      BranchGraphManager& branchMgr) {
    const char* items[] = {
        "我的账户信息",   // 1
        "我的银行卡",     // 2
        "我的交易记录",   // 3
        "排队取号",       // 4
        "网点查询导航",   // 5
        "注销登录"        // 6
    };
    while (true) {
        { char hdr[80]; snprintf(hdr, sizeof(hdr), "客户服务中心 - " SYSTEM_NAME); printHeader(hdr); }
        char subtitle[80];
        snprintf(subtitle, sizeof(subtitle), "当前用户: %s [客户]", g_session.userName);
        printInfo(subtitle);
        int c = showMenu("请选择服务", items, 6);
        if      (c == 1) custMgr.menuCustomer();
        else if (c == 2) cardMgr.menuCustomer();
        else if (c == 3) txnMgr.menuCustomer();
        else if (c == 4) queueMgr.menuCustomer();
        else if (c == 5) branchMgr.menuQuery();
        else if (c == 6 || c == 0) {
            g_session = Session();
            printInfo("已注销登录");
            return;
        }
    }
}

/*=============================================================
 *  程序入口
 *=============================================================*/
int main() {
    // 初始化控制台（设置 UTF-8 编码）
    system("chcp 65001 > nul");
    // 设置控制台标题
    SetConsoleTitle(TEXT(SYSTEM_NAME));

    // 确保数据目录存在
    ensureDataDir();

    // 初始化各管理器
    EmployeeManager   empMgr;
    CustomerManager   custMgr;
    BankCardManager   cardMgr;
    TransactionManager txnMgr(&cardMgr, &custMgr);
    BankQueueManager  queueMgr;
    BranchGraphManager branchMgr;
    SmartManager      smartMgr(&custMgr, &cardMgr, &txnMgr);

    // 初始化排队窗口（默认3普通+1VIP）
    queueMgr.initWindows(3, 1, &empMgr);

    // 主循环
    while (true) {
        showLoginMenu(empMgr, custMgr);
        if (!g_session.loggedIn()) break;  // 用户选择退出（选 0）

        switch (g_session.role) {
            case Role::ADMIN:
                adminMainMenu(empMgr, custMgr, cardMgr, txnMgr, queueMgr, branchMgr, smartMgr);
                break;
            case Role::EMPLOYEE:
                employeeMainMenu(empMgr, custMgr, cardMgr, txnMgr, queueMgr, branchMgr, smartMgr);
                break;
            case Role::CUSTOMER:
                customerMainMenu(custMgr, cardMgr, txnMgr, queueMgr, branchMgr);
                break;
            default:
                break;
        }
    }

    // 退出提示
    clearScreen();
    setColor(Color::LCYAN);
    printf("\n  感谢使用银行综合管理系统，再见！\n\n");
    resetColor();
    return 0;
}
