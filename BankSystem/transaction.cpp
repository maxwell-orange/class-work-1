#include "transaction.h"
#include "fileio.h"
#include "ui.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>

TransactionManager::TransactionManager(BankCardManager* cardMgr, CustomerManager* custMgr)
    : cardMgr_(cardMgr), custMgr_(custMgr) { load(); }
TransactionManager::~TransactionManager() { save(); }

/* ─── 构造交易记录 ──────────────────────────────────────────── */
Transaction TransactionManager::makeTransaction(
        const char* cardId, const char* custId, TxnType type,
        double amount, double balAfter, const char* handlerId,
        const char* desc, const char* targetCard) {
    Transaction t;
    generateId(t.id, MAX_ID, "T");
    strncpy(t.cardId,       cardId,     MAX_ID - 1);
    strncpy(t.customerId,   custId,     MAX_ID - 1);
    strncpy(t.targetCardId, targetCard, MAX_ID - 1);
    t.type        = type;
    t.amount      = amount;
    t.balanceAfter = balAfter;
    getCurrentTime(t.datetime, MAX_DATE);
    strncpy(t.description, desc, MAX_DESC - 1);
    strncpy(t.handlerId,   handlerId, MAX_ID - 1);
    t.isSuspicious = (amount >= LARGE_TXN_THRESHOLD);
    return t;
}

/* ─── 业务操作 ──────────────────────────────────────────────── */
bool TransactionManager::deposit(const char* cardId, double amount, const char* handlerId) {
    if (amount <= 0) return false;
    BankCard* c = cardMgr_->findByCardId(cardId);
    if (!c || c->isFrozen) return false;
    cardMgr_->updateBalance(cardId, amount);
    double bal = c->balance;
    char desc[MAX_DESC];
    snprintf(desc, MAX_DESC, "存款 %.2f 元", amount);
    list_.push_back(makeTransaction(cardId, c->customerId, TxnType::DEPOSIT, amount, bal, handlerId, desc));
    save();
    return true;
}

bool TransactionManager::withdraw(const char* cardId, double amount, const char* handlerId) {
    if (amount <= 0) return false;
    BankCard* c = cardMgr_->findByCardId(cardId);
    if (!c || c->isFrozen) return false;
    if (c->balance < amount) return false;
    cardMgr_->updateBalance(cardId, -amount);
    double bal = c->balance;
    char desc[MAX_DESC];
    snprintf(desc, MAX_DESC, "取款 %.2f 元", amount);
    list_.push_back(makeTransaction(cardId, c->customerId, TxnType::WITHDRAW, amount, bal, handlerId, desc));
    save();
    return true;
}

bool TransactionManager::transfer(const char* fromCardId, const char* toCardId,
                                  double amount, const char* handlerId) {
    if (amount <= 0) return false;
    BankCard* from = cardMgr_->findByCardId(fromCardId);
    BankCard* to   = cardMgr_->findByCardId(toCardId);
    if (!from || !to || from->isFrozen || to->isFrozen) return false;
    if (from->balance < amount) return false;
    cardMgr_->updateBalance(fromCardId, -amount);
    cardMgr_->updateBalance(toCardId,    amount);
    double bal = from->balance;
    char desc[MAX_DESC];
    snprintf(desc, MAX_DESC, "转账至 %s，金额 %.2f 元", toCardId, amount);
    list_.push_back(makeTransaction(fromCardId, from->customerId, TxnType::TRANSFER,
                                    amount, bal, handlerId, desc, toCardId));
    save();
    return true;
}

bool TransactionManager::applyLoan(const char* cardId, double amount, const char* handlerId) {
    if (amount <= 0) return false;
    BankCard* c = cardMgr_->findByCardId(cardId);
    if (!c || c->type != CardType::CREDIT) return false;
    cardMgr_->updateBalance(cardId, amount);
    cardMgr_->updateLoanBalance(cardId, amount);
    char desc[MAX_DESC];
    snprintf(desc, MAX_DESC, "贷款 %.2f 元", amount);
    list_.push_back(makeTransaction(cardId, c->customerId, TxnType::LOAN,
                                    amount, c->balance, handlerId, desc));
    save();
    return true;
}

bool TransactionManager::repayLoan(const char* cardId, double amount, const char* handlerId) {
    if (amount <= 0) return false;
    BankCard* c = cardMgr_->findByCardId(cardId);
    if (!c) return false;
    if (c->balance < amount) return false;
    if (amount > c->loanBalance) amount = c->loanBalance;
    cardMgr_->updateBalance(cardId, -amount);
    cardMgr_->updateLoanBalance(cardId, -amount);
    char desc[MAX_DESC];
    snprintf(desc, MAX_DESC, "还款 %.2f 元", amount);
    list_.push_back(makeTransaction(cardId, c->customerId, TxnType::REPAY,
                                    amount, c->balance, handlerId, desc));
    save();
    return true;
}

int TransactionManager::settleInterest(const char* handlerId) {
    int cnt = 0;
    cardMgr_->data().forEachMut([&](BankCard& card) {
        if (!card.isActive || card.isFrozen) return;
        double interest = 0.0;
        if (card.type == CardType::SAVINGS || card.type == CardType::DEBIT) {
            interest = card.balance * card.interestRate;
            if (interest > 0) {
                card.balance += interest;
                char desc[MAX_DESC];
                snprintf(desc, MAX_DESC, "月利息结算 %.2f 元（利率%.4f%%）",
                         interest, card.interestRate * 100);
                list_.push_back(makeTransaction(card.cardId, card.customerId,
                    TxnType::INTEREST, interest, card.balance, handlerId, desc));
                ++cnt;
            }
        } else if (card.type == CardType::CREDIT && card.loanBalance > 0) {
            // 信用卡贷款计息
            interest = card.loanBalance * card.interestRate;
            card.loanBalance += interest;
            char desc[MAX_DESC];
            snprintf(desc, MAX_DESC, "贷款利息 %.2f 元", interest);
            list_.push_back(makeTransaction(card.cardId, card.customerId,
                TxnType::INTEREST, interest, card.balance, handlerId, desc));
            ++cnt;
        }
    });
    cardMgr_->save();
    save();
    return cnt;
}

double TransactionManager::getBalance(const char* cardId) {
    BankCard* c = cardMgr_->findByCardId(cardId);
    return c ? c->balance : -1.0;
}

int TransactionManager::getHistory(const char* cardId, Transaction* out, int maxOut) {
    return list_.collect([&](const Transaction& t) {
        return strcmp(t.cardId, cardId) == 0;
    }, out, maxOut);
}

/* ─── 查询 ──────────────────────────────────────────────────── */
int TransactionManager::queryByDateRange(const char* start, const char* end,
                                          Transaction* out, int maxOut) {
    return list_.collect([&](const Transaction& t) {
        return strncmp(t.datetime, start, 10) >= 0 && strncmp(t.datetime, end, 10) <= 0;
    }, out, maxOut);
}

int TransactionManager::queryByCustType(CustType tp, Transaction* out, int maxOut,
                                         CustomerManager* cMgr) {
    return list_.collect([&](const Transaction& t) {
        Customer* c = cMgr->findById(t.customerId);
        return c != nullptr && c->type == tp;
    }, out, maxOut);
}

int TransactionManager::queryByTxnType(TxnType tp, Transaction* out, int maxOut) {
    return list_.collect([&](const Transaction& t) {
        return t.type == tp;
    }, out, maxOut);
}

int TransactionManager::queryByAmountRange(double lo, double hi,
                                            Transaction* out, int maxOut) {
    return list_.collect([&](const Transaction& t) {
        return t.amount >= lo && t.amount <= hi;
    }, out, maxOut);
}

void TransactionManager::summary(double& totalIn, double& totalOut,
                                  int& cntIn, int& cntOut) {
    totalIn = totalOut = 0; cntIn = cntOut = 0;
    list_.forEach([&](const Transaction& t) {
        if (t.type == TxnType::DEPOSIT || t.type == TxnType::INTEREST || t.type == TxnType::LOAN) {
            totalIn += t.amount; ++cntIn;
        } else {
            totalOut += t.amount; ++cntOut;
        }
    });
}

void TransactionManager::save() { saveList(TRANSACTION_FILE, list_); }
void TransactionManager::load() { loadList(TRANSACTION_FILE, list_); }

/* ─── 打印 ──────────────────────────────────────────────────── */
void TransactionManager::printTxnDetail(const Transaction& t) {
    printLine('-');
    printField("交易编号",   t.id);
    printField("卡    号",   t.cardId);
    printField("客户编号",   t.customerId);
    printField("交易类型",   txnTypeName(t.type));
    printFieldF("交易金额",  t.amount, 2);
    printFieldF("交易后余额",t.balanceAfter, 2);
    printField("交易时间",   t.datetime);
    printField("备    注",   t.description);
    if (t.isSuspicious) printWarning("⚠ 大额交易，已标记");
}

void TransactionManager::printTxnTable(const Transaction* arr, int cnt) {
    const char* cols[] = {"交易编号", "卡号", "类型", "金额(元)", "时间", "异常"};
    int widths[] = {14, 18, 8, 12, 20, 4};
    printTableHeader(cols, widths, 6);
    printTableSep(widths, 6);
    char amtStr[16];
    for (int i = 0; i < cnt; ++i) {
        snprintf(amtStr, sizeof(amtStr), "%.2f", arr[i].amount);
        const char* vals[] = {
            arr[i].id, arr[i].cardId, txnTypeName(arr[i].type),
            amtStr, arr[i].datetime, arr[i].isSuspicious ? "!" : ""
        };
        printTableRow(vals, widths, 6);
    }
    printTableSep(widths, 6);
}

void TransactionManager::printQueryResult(const Transaction* arr, int cnt) {
    if (cnt == 0) { printWarning("未找到匹配交易记录"); return; }
    char buf[48]; snprintf(buf, sizeof(buf), "共找到 %d 条记录", cnt);
    printInfo(buf);
    printTxnTable(arr, cnt);
}

/* ─── 交互式操作 ────────────────────────────────────────────── */
void TransactionManager::doDeposit() {
    printSubHeader("存款");
    extern Session g_session;
    char cardId[MAX_ID]; getInput("卡号: ", cardId, MAX_ID);
    BankCard* card = cardMgr_->findByCardId(cardId);
    if (!card) { printError("卡号不存在"); pause(); return; }
    printField("当前余额", "");
    printf("  %.2f 元\n", card->balance);
    double amt = getDoubleInput("存款金额(元): ", 0.01, 10000000.0);
    if (deposit(cardId, amt, g_session.userId)) {
        char buf[64]; snprintf(buf, sizeof(buf), "存款成功！当前余额 %.2f 元", card->balance);
        printSuccess(buf);
    } else printError("存款失败（卡已冻结或不存在）");
    pause();
}

void TransactionManager::doWithdraw() {
    printSubHeader("取款");
    extern Session g_session;
    char cardId[MAX_ID]; getInput("卡号: ", cardId, MAX_ID);
    BankCard* card = cardMgr_->findByCardId(cardId);
    if (!card) { printError("卡号不存在"); pause(); return; }
    printf("  当前余额: %.2f 元\n", card->balance);
    double amt = getDoubleInput("取款金额(元): ", 0.01, 10000000.0);
    if (withdraw(cardId, amt, g_session.userId)) {
        char buf[64]; snprintf(buf, sizeof(buf), "取款成功！当前余额 %.2f 元", card->balance);
        printSuccess(buf);
    } else printError("取款失败（余额不足或卡已冻结）");
    pause();
}

void TransactionManager::doTransfer() {
    printSubHeader("转账");
    extern Session g_session;
    char from[MAX_ID], to[MAX_ID];
    getInput("转出卡号: ", from, MAX_ID);
    getInput("转入卡号: ", to,   MAX_ID);
    BankCard* fc = cardMgr_->findByCardId(from);
    if (!fc) { printError("转出卡不存在"); pause(); return; }
    printf("  转出卡余额: %.2f 元\n", fc->balance);
    double amt = getDoubleInput("转账金额(元): ", 0.01, 10000000.0);
    if (transfer(from, to, amt, g_session.userId)) {
        char buf[64]; snprintf(buf, sizeof(buf), "转账成功！剩余余额 %.2f 元", fc->balance);
        printSuccess(buf);
    } else printError("转账失败（余额不足、卡号错误或卡已冻结）");
    pause();
}

void TransactionManager::doLoan() {
    printSubHeader("贷款申请");
    extern Session g_session;
    char cardId[MAX_ID]; getInput("信用卡卡号: ", cardId, MAX_ID);
    BankCard* c = cardMgr_->findByCardId(cardId);
    if (!c || c->type != CardType::CREDIT) { printError("请提供有效信用卡卡号"); pause(); return; }
    printf("  当前贷款余额: %.2f 元\n", c->loanBalance);
    double amt = getDoubleInput("贷款金额(元): ", 0.01, 1000000.0);
    if (applyLoan(cardId, amt, g_session.userId))
        printSuccess("贷款成功！金额已打入账户");
    else
        printError("贷款失败");
    pause();
}

void TransactionManager::doRepay() {
    printSubHeader("还款");
    extern Session g_session;
    char cardId[MAX_ID]; getInput("卡号: ", cardId, MAX_ID);
    BankCard* c = cardMgr_->findByCardId(cardId);
    if (!c) { printError("卡不存在"); pause(); return; }
    printf("  当前贷款余额: %.2f 元，账户余额: %.2f 元\n", c->loanBalance, c->balance);
    double amt = getDoubleInput("还款金额(元): ", 0.01, 1000000.0);
    if (repayLoan(cardId, amt, g_session.userId))
        printSuccess("还款成功！");
    else
        printError("还款失败（余额不足）");
    pause();
}

void TransactionManager::doInterest() {
    printSubHeader("月利息结算");
    extern Session g_session;
    if (!confirm("确认执行月利息结算？")) { printInfo("已取消"); pause(); return; }
    int cnt = settleInterest(g_session.userId);
    char buf[48]; snprintf(buf, sizeof(buf), "利息结算完成，共处理 %d 张卡", cnt);
    printSuccess(buf);
    pause();
}

void TransactionManager::doHistory() {
    printSubHeader("查询交易历史");
    char cardId[MAX_ID]; getInput("请输入卡号: ", cardId, MAX_ID);
    Transaction arr[200]; int cnt = getHistory(cardId, arr, 200);
    printQueryResult(arr, cnt);
    pause();
}

void TransactionManager::doQuery() {
    printSubHeader("业务查询");
    const char* opts[] = {
        "按时间段查询", "按客户类型查询", "按交易类型查询",
        "按金额区间查询", "汇总统计"
    };
    int choice = showMenu("查询方式", opts, 5);
    Transaction arr[500]; int cnt = 0;
    char buf[48];

    if (choice == 1) {
        char start[MAX_DATE], end[MAX_DATE];
        getInput("开始日期(YYYY-MM-DD): ", start, MAX_DATE);
        getInput("结束日期(YYYY-MM-DD): ", end,   MAX_DATE);
        cnt = queryByDateRange(start, end, arr, 500);
        printQueryResult(arr, cnt);
    } else if (choice == 2) {
        getInput("客户类型(1普通/2VIP): ", buf, 8);
        CustType ct = buf[0]=='2' ? CustType::VIP : CustType::NORMAL;
        cnt = queryByCustType(ct, arr, 500, custMgr_);
        printQueryResult(arr, cnt);
    } else if (choice == 3) {
        const char* typeOpts[] = {"存款","取款","转账","利息","贷款","还款"};
        int t = showMenu("交易类型", typeOpts, 6);
        if (t > 0) { cnt = queryByTxnType((TxnType)(t-1), arr, 500); printQueryResult(arr, cnt); }
    } else if (choice == 4) {
        double lo = getDoubleInput("最小金额(元): ", 0, 1e9);
        double hi = getDoubleInput("最大金额(元): ", lo, 1e9);
        cnt = queryByAmountRange(lo, hi, arr, 500);
        printQueryResult(arr, cnt);
    } else if (choice == 5) {
        doSummary(); return;
    } else return;
    pause();
}

void TransactionManager::doSummary() {
    printSubHeader("业务汇总统计");
    double totalIn, totalOut; int cntIn, cntOut;
    summary(totalIn, totalOut, cntIn, cntOut);
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f 元（%d笔）", totalIn, cntIn);
    printField("总收入",  buf);
    snprintf(buf, sizeof(buf), "%.2f 元（%d笔）", totalOut, cntOut);
    printField("总支出",  buf);
    snprintf(buf, sizeof(buf), "%d 笔", cntIn + cntOut);
    printField("总交易数", buf);
    pause();
}

void TransactionManager::doMyHistory() {
    extern Session g_session;
    printSubHeader("我的交易记录");
    // 找出客户的所有卡，汇总交易
    BankCard cards[32]; int cardCnt = cardMgr_->getCardsByCustomer(g_session.userId, cards, 32);
    Transaction arr[500]; int total = 0;
    for (int i = 0; i < cardCnt && total < 500; ++i) {
        Transaction tmp[100];
        int n = getHistory(cards[i].cardId, tmp, 100);
        for (int j = 0; j < n && total < 500; ++j) arr[total++] = tmp[j];
    }
    printQueryResult(arr, total);
    pause();
}

/* ─── 菜单入口 ──────────────────────────────────────────────── */
void TransactionManager::menuEmployee() {
    const char* items[] = {
        "存款", "取款", "转账",
        "贷款申请", "还款", "月利息结算",
        "查询交易历史", "业务查询 & 统计"
    };
    while (true) {
        printHeader("存贷款业务管理");
        int c = showMenu("请选择操作", items, 8);
        if      (c == 1) doDeposit();
        else if (c == 2) doWithdraw();
        else if (c == 3) doTransfer();
        else if (c == 4) doLoan();
        else if (c == 5) doRepay();
        else if (c == 6) doInterest();
        else if (c == 7) doHistory();
        else if (c == 8) doQuery();
        else             return;
    }
}

void TransactionManager::menuCustomer() {
    const char* items[] = {"查询我的交易记录", "查询账户余额"};
    while (true) {
        printHeader("我的账户业务");
        int c = showMenu("请选择操作", items, 2);
        if (c == 1) doMyHistory();
        else if (c == 2) {
            extern Session g_session;
            BankCard cards[32]; int cnt = cardMgr_->getCardsByCustomer(g_session.userId, cards, 32);
            printSubHeader("我的账户余额");
            for (int i = 0; i < cnt; ++i) {
                char buf[64]; snprintf(buf, sizeof(buf), "%.2f 元（%s，%s）",
                    cards[i].balance, cards[i].cardId, cardTypeName(cards[i].type));
                printField("卡", buf);
            }
            pause();
        } else return;
    }
}
