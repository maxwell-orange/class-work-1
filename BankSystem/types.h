#pragma once
#include <cstring>
#include <ctime>
#include <cstdio>
#include <cstdlib>

/*=============================================================
 *  全局常量
 *=============================================================*/
#define SYSTEM_NAME   "银行综合管理系统 v1.0"
#define DATA_DIR      "data\\"

static const int MAX_ID    = 24;
static const int MAX_NAME  = 52;
static const int MAX_PASS  = 64;
static const int MAX_PHONE = 24;
static const int MAX_ADDR  = 104;
static const int MAX_DATE  = 28;
static const int MAX_DESC  = 128;
static const int MAX_DEPT  = 52;
static const int MAX_BRANCHES = 50;

/*=============================================================
 *  枚举类型
 *=============================================================*/
enum class Role     { NONE, ADMIN, EMPLOYEE, CUSTOMER };
enum class CardType { DEBIT = 0, SAVINGS = 1, CREDIT = 2 };
enum class CustType { NORMAL = 0, VIP = 1 };
enum class TxnType  { DEPOSIT=0, WITHDRAW=1, TRANSFER=2, INTEREST=3, LOAN=4, REPAY=5 };

inline const char* cardTypeName(CardType t) {
    switch(t) {
        case CardType::DEBIT:   return "借记卡";
        case CardType::SAVINGS: return "储蓄卡";
        case CardType::CREDIT:  return "信用卡";
        default:                return "未知";
    }
}
inline const char* custTypeName(CustType t) { return t == CustType::VIP ? "VIP" : "普通"; }
inline const char* txnTypeName(TxnType t) {
    static const char* names[] = {"存款","取款","转账","利息","贷款","还款"};
    return names[(int)t];
}

/*=============================================================
 *  数据结构体
 *=============================================================*/

/* ─── 职员 ───────────────────────────────────────────────── */
struct Employee {
    char id[MAX_ID];
    char name[MAX_NAME];
    char password[MAX_PASS];
    char phone[MAX_PHONE];
    char department[MAX_DEPT];
    char position[MAX_NAME];
    char joinDate[MAX_DATE];
    bool isAdmin;
    bool isActive;

    Employee() { memset(this,0,sizeof(*this)); isActive=true; }
    bool operator==(const Employee& o) const { return strcmp(id,o.id)==0; }
};

/* ─── 客户 ───────────────────────────────────────────────── */
struct Customer {
    char id[MAX_ID];
    char name[MAX_NAME];
    char password[MAX_PASS];
    CustType type;
    char phone[MAX_PHONE];
    char address[MAX_ADDR];
    char openDate[MAX_DATE];
    double creditScore;       // 信用分 0-1000
    double financialAssets;   // 金融资产
    double monthlyIncome;     // 月收入
    double monthlyExpenses;   // 月支出
    int    cardCount;
    bool   isActive;

    Customer() {
        memset(this,0,sizeof(*this));
        type = CustType::NORMAL;
        creditScore = 600.0;
        isActive = true;
    }
    bool operator==(const Customer& o) const { return strcmp(id,o.id)==0; }
};

/* ─── 银行卡 ──────────────────────────────────────────────── */
struct BankCard {
    char cardId[MAX_ID];
    char customerId[MAX_ID];
    CardType type;
    double balance;
    double loanBalance;
    double interestRate;      // 月利率
    char openDate[MAX_DATE];
    bool isActive;
    bool isFrozen;

    BankCard() {
        memset(this,0,sizeof(*this));
        type = CardType::DEBIT;
        interestRate = 0.0035;
        isActive = true;
    }
    bool operator==(const BankCard& o) const { return strcmp(cardId,o.cardId)==0; }
};

/* ─── 交易记录 ────────────────────────────────────────────── */
struct Transaction {
    char id[MAX_ID];
    char cardId[MAX_ID];
    char customerId[MAX_ID];
    char targetCardId[MAX_ID];
    TxnType type;
    double amount;
    double balanceAfter;
    char datetime[MAX_DATE];
    char description[MAX_DESC];
    char handlerId[MAX_ID];
    bool isSuspicious;

    Transaction() { memset(this,0,sizeof(*this)); }
    bool operator==(const Transaction& o) const { return strcmp(id,o.id)==0; }
};

/* ─── 网点 ───────────────────────────────────────────────── */
struct Branch {
    char id[MAX_ID];
    char name[MAX_NAME];
    char address[MAX_ADDR];
    char phone[MAX_PHONE];
    double x, y;              // 坐标（km）
    bool isActive;

    Branch() { memset(this,0,sizeof(*this)); isActive=true; }
    bool operator==(const Branch& o) const { return strcmp(id,o.id)==0; }
};

/* ─── 排队票据 ────────────────────────────────────────────── */
struct QueueTicket {
    int      number;
    char     customerId[MAX_ID];
    char     customerName[MAX_NAME];
    CustType type;
    char     serviceType[MAX_NAME];
    char     arrivalTime[MAX_DATE];
    int      windowId;
    bool     served;
    int      rating;          // 0=未评价，1-5

    QueueTicket() { memset(this,0,sizeof(*this)); }
    // 优先队列比较：VIP > 普通，同类型号码小的优先
    bool operator<(const QueueTicket& o) const {
        if (type != o.type) return (int)type < (int)o.type;  // VIP(1) > NORMAL(0)
        return number > o.number;  // 号码小的优先
    }
};

/* ─── 窗口 ───────────────────────────────────────────────── */
struct Window {
    int    id;
    bool   isVIP;
    char   name[MAX_NAME];
    char   employeeId[MAX_ID];
    bool   isOpen;
    int    servedCount;
    double totalRating;

    Window() { memset(this,0,sizeof(*this)); }
};

/* ─── 会话（当前登录用户） ───────────────────────────────── */
struct Session {
    Role role;
    char userId[MAX_ID];
    char userName[MAX_NAME];

    Session() : role(Role::NONE) {
        memset(userId,0,sizeof(userId));
        memset(userName,0,sizeof(userName));
    }
    bool loggedIn() const { return role != Role::NONE; }
};

extern Session g_session;

/* ─── 工具函数 ────────────────────────────────────────────── */
inline void getCurrentTime(char* buf, int len) {
    time_t t = time(nullptr);
    struct tm* tm_info = localtime(&t);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm_info);
}
inline void getCurrentDate(char* buf, int len) {
    time_t t = time(nullptr);
    struct tm* tm_info = localtime(&t);
    strftime(buf, len, "%Y-%m-%d", tm_info);
}
// 生成简单ID（前缀+时间戳+随机数）
inline void generateId(char* buf, int len, const char* prefix) {
    static int counter = 0;
    time_t t = time(nullptr);
    snprintf(buf, len, "%s%08ld%04d", prefix, (long)t % 100000000, ++counter % 10000);
}
