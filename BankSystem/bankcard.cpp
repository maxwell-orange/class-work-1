#include "bankcard.h"
#include "fileio.h"
#include "ui.h"
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cstdlib>

BankCardManager::BankCardManager() { load(); }
BankCardManager::~BankCardManager() { save(); }

/* ─── 生成卡号 ─────────────────────────────────────────────── */
void BankCardManager::generateCardId(char* buf) {
    static int seq = 0;
    time_t t = time(nullptr);
    snprintf(buf, MAX_ID, "6222%04d%08ld", ++seq % 10000, (long)t % 100000000);
    buf[MAX_ID-1] = '\0';
}

/* ─── CRUD ─────────────────────────────────────────────────── */
bool BankCardManager::addCard(const BankCard& c) {
    if (findByCardId(c.cardId)) return false;
    list_.push_back(c);
    save();
    return true;
}

bool BankCardManager::deleteCard(const char* cardId) {
    BankCard* c = findByCardId(cardId);
    if (!c) return false;
    c->isActive = false;
    save();
    return true;
}

bool BankCardManager::updateCard(const BankCard& c) {
    BankCard* found = findByCardId(c.cardId);
    if (!found) return false;
    double bal = found->balance;
    double loan = found->loanBalance;
    *found = c;
    found->balance = bal;
    found->loanBalance = loan;
    save();
    return true;
}

bool BankCardManager::freezeCard(const char* cardId, bool freeze) {
    BankCard* c = findByCardId(cardId);
    if (!c) return false;
    c->isFrozen = freeze;
    save();
    return true;
}

BankCard* BankCardManager::findByCardId(const char* cardId) {
    return list_.find_if([&](const BankCard& c) {
        return c.isActive && strcmp(c.cardId, cardId) == 0;
    });
}

int BankCardManager::getCardsByCustomer(const char* customerId, BankCard* out, int maxOut) {
    return list_.collect([&](const BankCard& c) {
        return c.isActive && strcmp(c.customerId, customerId) == 0;
    }, out, maxOut);
}

int BankCardManager::queryByType(CardType t, BankCard* out, int maxOut) {
    return list_.collect([&](const BankCard& c) {
        return c.isActive && c.type == t;
    }, out, maxOut);
}

bool BankCardManager::updateBalance(const char* cardId, double delta) {
    BankCard* c = findByCardId(cardId);
    if (!c || c->isFrozen) return false;
    c->balance += delta;
    save();
    return true;
}

bool BankCardManager::updateLoanBalance(const char* cardId, double delta) {
    BankCard* c = findByCardId(cardId);
    if (!c) return false;
    c->loanBalance += delta;
    save();
    return true;
}

void BankCardManager::save() { saveList(CARD_FILE, list_); }
void BankCardManager::load() { loadList(CARD_FILE, list_); }

/* ─── 打印 ─────────────────────────────────────────────────── */
void BankCardManager::printCardDetail(const BankCard& c) {
    printLine('-');
    printField("卡    号",   c.cardId);
    printField("客户编号",   c.customerId);
    printField("卡    型",   cardTypeName(c.type));
    printFieldF("账户余额",  c.balance, 2);
    printFieldF("贷款余额",  c.loanBalance, 2);
    printFieldF("月利率(%)", c.interestRate * 100, 4);
    printField("开卡日期",   c.openDate);
    printField("冻结状态",   c.isFrozen ? "已冻结" : "正常");
}

void BankCardManager::printCardTable(const BankCard* arr, int cnt) {
    const char* cols[] = {"卡号", "客户编号", "卡型", "余额(元)", "贷款(元)", "状态"};
    int widths[] = {18, 14, 8, 14, 14, 8};
    printTableHeader(cols, widths, 6);
    printTableSep(widths, 6);
    char balStr[16], loanStr[16];
    for (int i = 0; i < cnt; ++i) {
        snprintf(balStr,  sizeof(balStr),  "%.2f", arr[i].balance);
        snprintf(loanStr, sizeof(loanStr), "%.2f", arr[i].loanBalance);
        const char* vals[] = {
            arr[i].cardId, arr[i].customerId, cardTypeName(arr[i].type),
            balStr, loanStr, arr[i].isFrozen ? "冻结" : "正常"
        };
        printTableRow(vals, widths, 6);
    }
    printTableSep(widths, 6);
}

void BankCardManager::listAll() {
    BankCard arr[1024]; int cnt = 0;
    list_.forEach([&](const BankCard& c) {
        if (c.isActive && cnt < 1024) arr[cnt++] = c;
    });
    printCardTable(arr, cnt);
}

/* ─── 交互式操作 ───────────────────────────────────────────── */
void BankCardManager::doAdd() {
    printSubHeader("开卡");
    BankCard c;
    generateCardId(c.cardId);
    getInput("客户编号: ", c.customerId, MAX_ID);
    const char* typeOpts[] = {"借记卡", "储蓄卡", "信用卡"};
    int t = showMenu("卡片类型", typeOpts, 3);
    if (t == 0) return;
    c.type = (CardType)(t - 1);
    if (c.type == CardType::CREDIT)
        c.interestRate = getDoubleInput("月利率(默认0.35输入0.0035): ", 0, 0.05);
    else
        c.interestRate = 0.0035;
    getCurrentDate(c.openDate, MAX_DATE);
    c.isActive = true;
    printField("生成卡号", c.cardId);
    if (addCard(c)) printSuccess("开卡成功！");
    else            printError("开卡失败！");
    pause();
}

void BankCardManager::doDelete() {
    printSubHeader("销卡");
    char id[MAX_ID]; getInput("请输入卡号: ", id, MAX_ID);
    BankCard* c = findByCardId(id);
    if (!c) { printError("卡不存在！"); pause(); return; }
    printCardDetail(*c);
    if (c->balance > 0) { printWarning("卡内有余额，请先取出再销卡！"); pause(); return; }
    if (!confirm("确认销卡？")) { printInfo("已取消"); pause(); return; }
    if (deleteCard(id)) printSuccess("销卡成功");
    else                printError("销卡失败");
    pause();
}

void BankCardManager::doUpdate() {
    printSubHeader("修改卡片信息");
    char id[MAX_ID]; getInput("请输入卡号: ", id, MAX_ID);
    BankCard* c = findByCardId(id);
    if (!c) { printError("卡不存在！"); pause(); return; }
    printCardDetail(*c);
    BankCard nc = *c;
    char buf[32];
    getInput("新月利率(空跳过): ", buf, sizeof(buf));
    if (buf[0]) sscanf(buf, "%lf", &nc.interestRate);
    if (updateCard(nc)) printSuccess("修改成功！");
    else                printError("修改失败！");
    pause();
}

void BankCardManager::doQuery() {
    printSubHeader("查询银行卡");
    const char* opts[] = {"按卡号查询", "按客户编号查询", "按卡类型查询"};
    int choice = showMenu("查询方式", opts, 3);
    BankCard arr[64]; int cnt = 0;
    char buf[MAX_ID];
    if (choice == 1) {
        getInput("请输入卡号: ", buf, MAX_ID);
        BankCard* c = findByCardId(buf);
        if (c) { arr[0]=*c; cnt=1; }
    } else if (choice == 2) {
        getInput("请输入客户编号: ", buf, MAX_ID);
        cnt = getCardsByCustomer(buf, arr, 64);
    } else if (choice == 3) {
        const char* typeOpts[] = {"借记卡", "储蓄卡", "信用卡"};
        int t = showMenu("卡类型", typeOpts, 3);
        if (t > 0) cnt = queryByType((CardType)(t-1), arr, 64);
    } else return;
    if (cnt == 0) { printWarning("未找到匹配银行卡"); }
    else {
        char numBuf[32]; snprintf(numBuf, sizeof(numBuf), "共找到 %d 张银行卡", cnt);
        printInfo(numBuf);
        printCardTable(arr, cnt);
        if (cnt == 1) { printCardDetail(arr[0]); }
    }
    pause();
}

void BankCardManager::doList() {
    printSubHeader("全部银行卡列表");
    listAll();
    int total = 0;
    list_.forEach([&](const BankCard& c) { if(c.isActive) ++total; });
    char buf[32]; snprintf(buf, sizeof(buf), "共 %d 张有效银行卡", total);
    printInfo(buf);
    pause();
}

void BankCardManager::doFreeze() {
    printSubHeader("冻结/解冻银行卡");
    char id[MAX_ID]; getInput("请输入卡号: ", id, MAX_ID);
    BankCard* c = findByCardId(id);
    if (!c) { printError("卡不存在！"); pause(); return; }
    bool freeze = !c->isFrozen;
    if (freezeCard(id, freeze))
        printSuccess(freeze ? "卡已冻结" : "卡已解冻");
    else
        printError("操作失败");
    pause();
}

/* ─── 菜单入口 ─────────────────────────────────────────────── */
void BankCardManager::menuEmployee() {
    const char* items[] = {"开卡", "销卡", "修改卡信息", "冻结/解冻", "查询银行卡", "显示所有银行卡"};
    while (true) {
        printHeader("银行卡管理");
        int c = showMenu("请选择操作", items, 6);
        if      (c == 1) doAdd();
        else if (c == 2) doDelete();
        else if (c == 3) doUpdate();
        else if (c == 4) doFreeze();
        else if (c == 5) doQuery();
        else if (c == 6) doList();
        else             return;
    }
}

void BankCardManager::menuCustomer() {
    extern Session g_session;
    const char* items[] = {"查看我的银行卡", "查看卡详情"};
    while (true) {
        printHeader("我的银行卡");
        int c = showMenu("请选择操作", items, 2);
        if (c == 1) {
            BankCard arr[32]; int cnt = getCardsByCustomer(g_session.userId, arr, 32);
            if (cnt == 0) printWarning("您暂无银行卡");
            else          printCardTable(arr, cnt);
            pause();
        } else if (c == 2) {
            char id[MAX_ID]; getInput("请输入卡号: ", id, MAX_ID);
            BankCard* card = findByCardId(id);
            if (!card || strcmp(card->customerId, g_session.userId) != 0)
                printError("卡不存在或不属于您");
            else
                printCardDetail(*card);
            pause();
        } else return;
    }
}
