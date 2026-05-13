#pragma once
#include "types.h"
#include "datastructs.h"
#include "bankcard.h"
#include "customer.h"

/*=============================================================
 *  模块4：存贷款业务管理
 *  模块5：业务查询
 *=============================================================*/

#define TRANSACTION_FILE "data\\transactions.dat"

// 大额交易阈值（元）
static const double LARGE_TXN_THRESHOLD = 50000.0;

class TransactionManager {
public:
    TransactionManager(BankCardManager* cardMgr, CustomerManager* custMgr);
    ~TransactionManager();

    // ── 模块4：业务操作 ────────────────────────────────────
    bool deposit(const char* cardId, double amount, const char* handlerId);
    bool withdraw(const char* cardId, double amount, const char* handlerId);
    bool transfer(const char* fromCardId, const char* toCardId,
                  double amount, const char* handlerId);
    bool applyLoan(const char* cardId, double amount, const char* handlerId);
    bool repayLoan(const char* cardId, double amount, const char* handlerId);
    // 月利息结算（所有卡）
    int  settleInterest(const char* handlerId);
    // 查询账户余额
    double getBalance(const char* cardId);

    // 获取某张卡的交易历史
    int getHistory(const char* cardId, Transaction* out, int maxOut);

    // ── 模块5：业务查询 ───────────────────────────────────
    // 按时间段
    int queryByDateRange(const char* start, const char* end, Transaction* out, int maxOut);
    // 按客户类型
    int queryByCustType(CustType t, Transaction* out, int maxOut,
                        CustomerManager* cMgr);
    // 按交易类型
    int queryByTxnType(TxnType t, Transaction* out, int maxOut);
    // 按金额区间
    int queryByAmountRange(double lo, double hi, Transaction* out, int maxOut);
    // 统计摘要：收入/支出总额、笔数
    void summary(double& totalIn, double& totalOut, int& cntIn, int& cntOut);

    void save();
    void load();

    void menuEmployee();
    void menuCustomer();

    LinkedList<Transaction>& data() { return list_; }

private:
    LinkedList<Transaction> list_;
    BankCardManager* cardMgr_;
    CustomerManager* custMgr_;

    Transaction makeTransaction(const char* cardId, const char* custId,
                                TxnType type, double amount, double balAfter,
                                const char* handlerId, const char* desc,
                                const char* targetCard = "");

    void printTxnDetail(const Transaction& t);
    void printTxnTable(const Transaction* arr, int cnt);
    void printQueryResult(const Transaction* arr, int cnt);

    void doDeposit();
    void doWithdraw();
    void doTransfer();
    void doLoan();
    void doRepay();
    void doInterest();
    void doHistory();
    void doQuery();
    void doSummary();
    void doMyHistory();
};
