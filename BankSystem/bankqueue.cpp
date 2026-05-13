#include "bankqueue.h"
#include "fileio.h"
#include "ui.h"
#include <cstring>
#include <cstdio>

BankQueueManager::BankQueueManager()
    : windowCount_(0), nextTicketNo_(1), todayServed_(0), todayRatingSum_(0) {
    memset(windows_, 0, sizeof(windows_));
    loadStats();
}
BankQueueManager::~BankQueueManager() { saveStats(); }

/* ─── 窗口初始化 ────────────────────────────────────────────── */
void BankQueueManager::initWindows(int normalCnt, int vipCnt, EmployeeManager*) {
    windowCount_ = 0;
    for (int i = 0; i < normalCnt && windowCount_ < MAX_WINDOWS; ++i) {
        Window& w = windows_[windowCount_++];
        w.id = windowCount_;
        w.isVIP = false;
        snprintf(w.name, MAX_NAME, "普通窗口 %d", windowCount_);
        w.isOpen = false;
    }
    for (int i = 0; i < vipCnt && windowCount_ < MAX_WINDOWS; ++i) {
        Window& w = windows_[windowCount_++];
        w.id = windowCount_;
        w.isVIP = true;
        snprintf(w.name, MAX_NAME, "VIP窗口 %d", windowCount_);
        w.isOpen = false;
    }
}

bool BankQueueManager::openWindow(int winId, const char* employeeId) {
    for (int i = 0; i < windowCount_; ++i) {
        if (windows_[i].id == winId) {
            windows_[i].isOpen = true;
            strncpy(windows_[i].employeeId, employeeId, MAX_ID - 1);
            return true;
        }
    }
    return false;
}

bool BankQueueManager::closeWindow(int winId) {
    for (int i = 0; i < windowCount_; ++i) {
        if (windows_[i].id == winId) {
            windows_[i].isOpen = false;
            memset(windows_[i].employeeId, 0, MAX_ID);
            return true;
        }
    }
    return false;
}

void BankQueueManager::listWindows() {
    const char* cols[] = {"窗口ID", "名称", "类型", "状态", "今日服务", "平均评分"};
    int widths[] = {8, 18, 8, 8, 10, 10};
    printTableHeader(cols, widths, 6);
    printTableSep(widths, 6);
    char idStr[8], servedStr[8], ratingStr[12];
    for (int i = 0; i < windowCount_; ++i) {
        snprintf(idStr,     sizeof(idStr),     "%d",    windows_[i].id);
        snprintf(servedStr, sizeof(servedStr),  "%d",   windows_[i].servedCount);
        double avg = windows_[i].servedCount > 0
            ? windows_[i].totalRating / windows_[i].servedCount : 0.0;
        snprintf(ratingStr, sizeof(ratingStr), "%.1f", avg);
        const char* vals[] = {
            idStr, windows_[i].name,
            windows_[i].isVIP ? "VIP" : "普通",
            windows_[i].isOpen ? "开放" : "关闭",
            servedStr, ratingStr
        };
        printTableRow(vals, widths, 6);
    }
    printTableSep(widths, 6);
}

/* ─── 取号 ──────────────────────────────────────────────────── */
QueueTicket BankQueueManager::takeNumber(const char* customerId, const char* name,
                                          CustType type, const char* serviceType) {
    QueueTicket t;
    t.number = nextTicketNo_++;
    strncpy(t.customerId,    customerId,   MAX_ID   - 1);
    strncpy(t.customerName,  name,         MAX_NAME - 1);
    t.type = type;
    strncpy(t.serviceType,   serviceType,  MAX_NAME - 1);
    getCurrentTime(t.arrivalTime, MAX_DATE);
    t.served = false;
    t.rating = 0;

    if (type == CustType::VIP)
        vipQueue_.push(t);
    else
        normalQueue_.enqueue(t);

    return t;
}

/* ─── 叫号（VIP 优先） ──────────────────────────────────────── */
bool BankQueueManager::callNext(int windowId, QueueTicket& out) {
    Window* w = nullptr;
    for (int i = 0; i < windowCount_; ++i) {
        if (windows_[i].id == windowId && windows_[i].isOpen) { w = &windows_[i]; break; }
    }
    if (!w) return false;

    bool got = false;
    if (w->isVIP) {
        // VIP 窗口：先从 VIP 队取，没有则取普通队
        if (!vipQueue_.empty()) { out = vipQueue_.pop(); got = true; }
        else if (!normalQueue_.empty()) { out = normalQueue_.dequeue(); got = true; }
    } else {
        // 普通窗口：先从普通队取，没有则取 VIP 队
        if (!normalQueue_.empty()) { out = normalQueue_.dequeue(); got = true; }
        else if (!vipQueue_.empty()) { out = vipQueue_.pop(); got = true; }
    }
    if (!got) return false;

    out.windowId = windowId;
    out.served   = true;
    ++w->servedCount;
    ++todayServed_;
    done_.push_back(out);
    return true;
}

bool BankQueueManager::rateService(int ticketNumber, int rating) {
    if (rating < 1 || rating > 5) return false;
    QueueTicket* t = findDoneTicket(ticketNumber);
    if (!t || t->rating != 0) return false;
    t->rating = rating;
    // 更新窗口评分
    for (int i = 0; i < windowCount_; ++i) {
        if (windows_[i].id == t->windowId) {
            windows_[i].totalRating += rating;
            break;
        }
    }
    todayRatingSum_ += rating;
    return true;
}

QueueTicket* BankQueueManager::findDoneTicket(int number) {
    return done_.find_if([&](const QueueTicket& t) {
        return t.number == number;
    });
}

/* ─── 显示队列 ──────────────────────────────────────────────── */
void BankQueueManager::displayQueues() {
    printSubHeader("当前等待队列");
    setColor(Color::LYELLOW);
    printf("\n  [VIP队列] 等待人数: %d\n", vipQueue_.size());
    resetColor();
    vipQueue_.forEach([](const QueueTicket& t, int i) {
        setColor(Color::LGREEN);
        printf("    %d. 号码:%04d  %s  业务:%s\n",
               i+1, t.number, t.customerName, t.serviceType);
        resetColor();
    });

    setColor(Color::LYELLOW);
    printf("\n  [普通队列] 等待人数: %d\n", normalQueue_.size());
    resetColor();
    normalQueue_.forEach([](const QueueTicket& t, int i) {
        printf("    %d. 号码:%04d  %s  业务:%s\n",
               i+1, t.number, t.customerName, t.serviceType);
    });

    printf("\n");
    listWindows();
}

/* ─── 每日报告 ──────────────────────────────────────────────── */
void BankQueueManager::dailyReport() {
    printSubHeader("今日排队统计");
    printFieldI("今日服务总人数", todayServed_);
    double avgRating = todayServed_ > 0 ? todayRatingSum_ / todayServed_ : 0;
    printFieldF("平均评分",       avgRating, 2);

    // 保存统计
    DailyStat ds;
    getCurrentDate(ds.date, MAX_DATE);
    ds.totalServed = todayServed_;
    ds.avgRating   = avgRating;
    ds.vipServed   = ds.normalServed = 0;
    done_.forEach([&](const QueueTicket& t) {
        if (t.type == CustType::VIP) ++ds.vipServed;
        else ++ds.normalServed;
    });
    printFieldI("VIP 客户服务数",  ds.vipServed);
    printFieldI("普通客户服务数",  ds.normalServed);
    stats_.push_back(ds);
    saveStats();
}

void BankQueueManager::saveStats() { saveList(QUEUE_STAT_FILE, stats_); }
void BankQueueManager::loadStats() { loadList(QUEUE_STAT_FILE, stats_); }

/* ─── 交互操作 ──────────────────────────────────────────────── */
void BankQueueManager::printWindowStatus() {
    printf("\n");
    listWindows();
}

void BankQueueManager::doTakeNumber() {
    printSubHeader("客户取号");
    extern Session g_session;
    char custId[MAX_ID] = {0}, custName[MAX_NAME] = {0};
    char svcType[MAX_NAME];

    // 若当前是客户身份，自动填充
    if (g_session.role == Role::CUSTOMER) {
        strncpy(custId, g_session.userId, MAX_ID-1);
        strncpy(custName, g_session.userName, MAX_NAME-1);
    } else {
        getInput("客户编号: ", custId, MAX_ID);
        getInput("客户姓名: ", custName, MAX_NAME);
    }

    const char* svcOpts[] = {"存款/取款","转账","开卡/销卡","贷款/还款","咨询","其他"};
    int s = showMenu("业务类型", svcOpts, 6);
    if (s == 0) return;
    strncpy(svcType, svcOpts[s-1], MAX_NAME-1);

    char typeStr[8]; getInput("客户类型(1普通/2VIP): ", typeStr, 8);
    CustType ct = typeStr[0]=='2' ? CustType::VIP : CustType::NORMAL;

    QueueTicket ticket = takeNumber(custId, custName, ct, svcType);
    char buf[64];
    snprintf(buf, sizeof(buf), "取号成功！您的号码是 %04d（%s）",
             ticket.number, ct == CustType::VIP ? "VIP" : "普通");
    printSuccess(buf);
    char wBuf[64];
    snprintf(wBuf, sizeof(wBuf), "当前等待人数: VIP=%d 普通=%d",
             vipQueue_.size(), normalQueue_.size());
    printInfo(wBuf);
    pause();
}

void BankQueueManager::doCallNext() {
    printSubHeader("叫下一号");
    listWindows();
    int winId = getIntInput("请输入窗口编号: ", 1, MAX_WINDOWS);
    QueueTicket ticket;
    if (callNext(winId, ticket)) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "请 %04d 号（%s）到 窗口%d 办理 [%s]",
                 ticket.number, ticket.customerName, winId, ticket.serviceType);
        setColor(Color::LGREEN);
        printf("\n  ★ %s\n\n", buf);
        resetColor();
    } else {
        printWarning("当前无等待客户");
    }
    pause();
}

void BankQueueManager::doRate() {
    printSubHeader("服务评分");
    int num = getIntInput("请输入您的号码: ", 1, 99999);
    int rating = getIntInput("请为本次服务评分(1-5): ", 1, 5);
    if (rateService(num, rating)) printSuccess("感谢您的评价！");
    else printError("评价失败（号码无效或已评价）");
    pause();
}

void BankQueueManager::doOpenClose() {
    printSubHeader("开启/关闭窗口");
    listWindows();
    int winId = getIntInput("请输入窗口编号: ", 1, MAX_WINDOWS);
    bool found = false;
    for (int i = 0; i < windowCount_; ++i) {
        if (windows_[i].id == winId) { found = true; break; }
    }
    if (!found) { printError("窗口不存在"); pause(); return; }
    const char* opts[] = {"开启窗口", "关闭窗口"};
    int c = showMenu("操作", opts, 2);
    if (c == 1) {
        extern Session g_session;
        if (openWindow(winId, g_session.userId)) printSuccess("窗口已开启");
        else printError("操作失败");
    } else if (c == 2) {
        if (closeWindow(winId)) printSuccess("窗口已关闭");
        else printError("操作失败");
    }
    pause();
}

/* ─── 菜单入口 ──────────────────────────────────────────────── */
void BankQueueManager::menuAdmin() {
    const char* items[] = {
        "初始化窗口", "开启/关闭窗口",
        "显示队列状态", "今日统计报告"
    };
    while (true) {
        printHeader("排队管理 - 管理员");
        int c = showMenu("请选择操作", items, 4);
        if (c == 1) {
            int nc = getIntInput("普通窗口数量(1-8): ", 1, 8);
            int vc = getIntInput("VIP窗口数量(0-4): ",  0, 4);
            initWindows(nc, vc, nullptr);
            printSuccess("窗口初始化完成");
            listWindows();
            pause();
        } else if (c == 2) doOpenClose();
        else if (c == 3) { printHeader("队列状态"); displayQueues(); pause(); }
        else if (c == 4) { dailyReport(); pause(); }
        else return;
    }
}

void BankQueueManager::menuEmployee(EmployeeManager*) {
    const char* items[] = {
        "叫下一号", "客户取号（代操作）",
        "显示队列状态", "开启/关闭本窗口"
    };
    while (true) {
        printHeader("排队管理 - 职员");
        int c = showMenu("请选择操作", items, 4);
        if      (c == 1) doCallNext();
        else if (c == 2) doTakeNumber();
        else if (c == 3) { printHeader("队列状态"); displayQueues(); pause(); }
        else if (c == 4) doOpenClose();
        else             return;
    }
}

void BankQueueManager::menuCustomer() {
    const char* items[] = {"取号排队", "查看队列状态", "服务评分"};
    while (true) {
        printHeader("排队取号 - 客户");
        int c = showMenu("请选择操作", items, 3);
        if      (c == 1) doTakeNumber();
        else if (c == 2) { printHeader("队列状态"); displayQueues(); pause(); }
        else if (c == 3) doRate();
        else             return;
    }
}
