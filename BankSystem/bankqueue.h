#pragma once
#include "types.h"
#include "datastructs.h"
#include "employee.h"

/*=============================================================
 *  模块6：银行排队管理
 *  - 普通窗口 + VIP 窗口
 *  - VIP 客户优先
 *  - 呼号 → 服务 → 评分
 *  - 每日统计
 *=============================================================*/

#define QUEUE_STAT_FILE "data\\queue_stats.dat"

// 每日统计
struct DailyStat {
    char   date[MAX_DATE];
    int    totalServed;
    double avgRating;
    int    vipServed;
    int    normalServed;
};

class BankQueueManager {
public:
    static const int MAX_WINDOWS = 10;

    BankQueueManager();
    ~BankQueueManager();

    // ── 窗口管理 ──────────────────────────────────────────
    void initWindows(int normalCnt, int vipCnt, EmployeeManager* empMgr);
    bool openWindow(int winId,  const char* employeeId);
    bool closeWindow(int winId);
    void listWindows();

    // ── 取号 / 叫号 ───────────────────────────────────────
    QueueTicket takeNumber(const char* customerId, const char* name,
                           CustType type, const char* serviceType);
    bool callNext(int windowId, QueueTicket& out);  // 叫下一号
    bool rateService(int ticketNumber, int rating);  // 客户评分

    // ── 显示队列 ──────────────────────────────────────────
    void displayQueues();

    // ── 统计 ──────────────────────────────────────────────
    void dailyReport();

    // ── 持久化 ────────────────────────────────────────────
    void saveStats();
    void loadStats();

    // ── UI 入口 ───────────────────────────────────────────
    void menuAdmin();
    void menuEmployee(EmployeeManager* empMgr);
    void menuCustomer();

private:
    // VIP 使用优先队列，普通使用普通队列
    PriorityQueue<QueueTicket> vipQueue_;
    Queue<QueueTicket>          normalQueue_;
    Window windows_[MAX_WINDOWS];
    int    windowCount_;
    int    nextTicketNo_;
    int    todayServed_;
    double todayRatingSum_;
    LinkedList<DailyStat> stats_;
    // 已完成的票据（用于评分）
    LinkedList<QueueTicket> done_;

    QueueTicket* findDoneTicket(int number);
    void printWindowStatus();
    void doTakeNumber();
    void doCallNext();
    void doRate();
    void doOpenClose();
};
