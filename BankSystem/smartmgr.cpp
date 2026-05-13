#include "smartmgr.h"
#include "ui.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>

SmartManager::SmartManager(CustomerManager* cMgr, BankCardManager* cardMgr,
                            TransactionManager* txnMgr)
    : cMgr_(cMgr), cardMgr_(cardMgr), txnMgr_(txnMgr) {}

/* ─── 8.1 异常交易检测 ──────────────────────────────────────── */
int SmartManager::detectAnomalies(Transaction* out, int maxOut) {
    // 收集大额交易
    int cnt = txnMgr_->data().collect([](const Transaction& t) {
        return t.isSuspicious;
    }, out, maxOut);
    // 检测频繁交易（同一卡同一小时内 >= 5 笔）
    // 利用栈统计
    static const int FREQ_THRESHOLD = 5;
    // 简单扫描：对每张卡统计当日交易笔数
    BankCard cards[1024]; int cardCnt = 0;
    cardMgr_->data().forEach([&](const BankCard& c) {
        if (c.isActive && cardCnt < 1024) cards[cardCnt++] = c;
    });
    for (int i = 0; i < cardCnt && cnt < maxOut; ++i) {
        char today[MAX_DATE]; getCurrentDate(today, MAX_DATE);
        int dayCnt = 0;
        txnMgr_->data().forEach([&](const Transaction& t) {
            if (strcmp(t.cardId, cards[i].cardId) == 0 &&
                strncmp(t.datetime, today, 10) == 0) ++dayCnt;
        });
        if (dayCnt >= FREQ_THRESHOLD) {
            // 添加一条汇总异常记录
            Transaction fake;
            strncpy(fake.cardId,     cards[i].cardId,     MAX_ID-1);
            strncpy(fake.customerId, cards[i].customerId, MAX_ID-1);
            fake.type = TxnType::DEPOSIT;
            fake.amount = 0;
            fake.isSuspicious = true;
            char desc[MAX_DESC];
            snprintf(desc, sizeof(desc), "⚠ 今日交易 %d 笔（频繁交易预警）", dayCnt);
            strncpy(fake.description, desc, MAX_DESC-1);
            getCurrentDate(fake.datetime, MAX_DATE);
            out[cnt++] = fake;
        }
    }
    return cnt;
}

void SmartManager::reportAnomalies() {
    printSubHeader("交易异常报告");
    Transaction arr[200]; int cnt = detectAnomalies(arr, 200);
    if (cnt == 0) { printSuccess("暂无异常交易记录"); return; }
    char buf[48]; snprintf(buf, sizeof(buf), "检测到 %d 条异常记录", cnt);
    printWarning(buf);
    const char* cols[] = {"卡号", "客户", "金额(元)", "时间/说明"};
    int widths[] = {18, 14, 14, 30};
    printTableHeader(cols, widths, 4);
    printTableSep(widths, 4);
    char amtStr[16];
    for (int i = 0; i < cnt; ++i) {
        snprintf(amtStr, sizeof(amtStr), "%.2f", arr[i].amount);
        const char* desc = arr[i].description[0] ? arr[i].description : arr[i].datetime;
        const char* vals[] = {arr[i].cardId, arr[i].customerId, amtStr, desc};
        printTableRow(vals, widths, 4);
    }
    printTableSep(widths, 4);
}

/* ─── 8.2 信用评级 ──────────────────────────────────────────── */
double SmartManager::calcCreditScore(const Customer& c, BankCardManager* cardMgr,
                                      TransactionManager* txnMgr) {
    double score = 600.0;

    // 金融资产加分
    if      (c.financialAssets > 500000) score += 150;
    else if (c.financialAssets > 100000) score += 80;
    else if (c.financialAssets > 10000)  score += 30;

    // 收支比加分/减分
    if (c.monthlyIncome > 0) {
        double ratio = c.monthlyExpenses / c.monthlyIncome;
        if (ratio < 0.3)      score += 60;
        else if (ratio < 0.5) score += 30;
        else if (ratio > 0.9) score -= 50;
    }

    // 检查贷款余额
    BankCard cards[32]; int cnt = cardMgr->getCardsByCustomer(c.id, cards, 32);
    double totalLoan = 0;
    for (int i = 0; i < cnt; ++i) totalLoan += cards[i].loanBalance;
    if (totalLoan > c.monthlyIncome * 36) score -= 100;
    else if (totalLoan > c.monthlyIncome * 12) score -= 50;

    // 历史大额异常交易减分
    Transaction txns[100]; int txnCnt = 0;
    for (int i = 0; i < cnt; ++i) {
        Transaction tmp[50];
        int n = txnMgr->getHistory(cards[i].cardId, tmp, 50);
        for (int j = 0; j < n && txnCnt < 100; ++j) {
            if (tmp[j].isSuspicious) { txns[txnCnt++] = tmp[j]; score -= 5; }
        }
    }

    if (score < 0)    score = 0;
    if (score > 1000) score = 1000;
    return score;
}

void SmartManager::updateAllCreditScores() {
    cMgr_->data().forEachMut([&](Customer& c) {
        if (!c.isActive) return;
        c.creditScore = calcCreditScore(c, cardMgr_, txnMgr_);
    });
    cMgr_->save();
}

bool SmartManager::loanApproval(const char* customerId, double amount,
                                 char* reason, int reasonLen) {
    Customer* c = cMgr_->findById(customerId);
    if (!c) { snprintf(reason, reasonLen, "客户不存在"); return false; }

    double score = c->creditScore;
    // 风控规则
    if (score < 400) {
        snprintf(reason, reasonLen, "信用分不足（%.0f < 400），拒绝贷款", score);
        return false;
    }
    double maxLoan = c->monthlyIncome * 24;  // 最大贷款为24个月月收入
    if (amount > maxLoan) {
        snprintf(reason, reasonLen, "贷款金额超过上限（%.0f元），拒绝", maxLoan);
        return false;
    }
    double loanRatio = c->monthlyExpenses > 0
        ? (amount / 24) / c->monthlyIncome : 0;
    if (loanRatio > 0.5) {
        snprintf(reason, reasonLen, "月还款额超过月收入50%%，建议降低贷款金额");
        return false;
    }
    snprintf(reason, reasonLen,
             "审批通过（信用分%.0f，建议利率%.2f%%/月）",
             score, score > 700 ? 0.3 : score > 500 ? 0.5 : 0.8);
    return true;
}

/* ─── 8.3 客户统计分析 ──────────────────────────────────────── */
void SmartManager::customerStatistics() {
    printSubHeader("客户统计分析");
    int total = 0, vip = 0, normal = 0;
    double avgAsset = 0, avgCredit = 0, avgIncome = 0;
    cMgr_->data().forEach([&](const Customer& c) {
        if (!c.isActive) return;
        ++total;
        if (c.type == CustType::VIP) ++vip; else ++normal;
        avgAsset  += c.financialAssets;
        avgCredit += c.creditScore;
        avgIncome += c.monthlyIncome;
    });
    if (total == 0) { printWarning("暂无客户数据"); return; }
    avgAsset  /= total;
    avgCredit /= total;
    avgIncome /= total;

    printFieldI("总客户数",   total);
    printFieldI("VIP 客户",   vip);
    printFieldI("普通客户",   normal);
    printFieldF("平均金融资产（元）", avgAsset, 2);
    printFieldF("平均信用分",   avgCredit, 1);
    printFieldF("平均月收入（元）", avgIncome, 2);

    // 信用分分布
    printSubHeader("信用分分布");
    int lvl[5] = {};  // <400, 400-599, 600-699, 700-799, >=800
    cMgr_->data().forEach([&](const Customer& c) {
        if (!c.isActive) return;
        double s = c.creditScore;
        if      (s < 400) ++lvl[0];
        else if (s < 600) ++lvl[1];
        else if (s < 700) ++lvl[2];
        else if (s < 800) ++lvl[3];
        else              ++lvl[4];
    });
    const char* labels[] = {"<400(差)","400-599(一般)","600-699(中)","700-799(良)",">=800(优)"};
    for (int i = 0; i < 5; ++i) {
        char buf[48]; snprintf(buf, sizeof(buf), "%d 人", lvl[i]);
        printField(labels[i], buf);
    }
}

void SmartManager::assetDistribution() {
    printSubHeader("资产分布统计");
    double total = 0; int cnt = 0;
    cMgr_->data().forEach([&](const Customer& c) {
        if (c.isActive) { total += c.financialAssets; ++cnt; }
    });
    if (cnt == 0) { printWarning("暂无数据"); return; }
    // 卡余额总计
    double cardTotal = 0;
    cardMgr_->data().forEach([&](const BankCard& c) {
        if (c.isActive) cardTotal += c.balance;
    });
    double loanTotal = 0;
    cardMgr_->data().forEach([&](const BankCard& c) {
        if (c.isActive) loanTotal += c.loanBalance;
    });

    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f 元", total);     printField("客户金融资产总计", buf);
    snprintf(buf, sizeof(buf), "%.2f 元", cardTotal); printField("银行卡存款总计",   buf);
    snprintf(buf, sizeof(buf), "%.2f 元", loanTotal); printField("贷款余额总计",     buf);
    snprintf(buf, sizeof(buf), "%.2f 元", total / cnt); printField("人均金融资产",   buf);
}

/* ─── 8.4 利息报表 ──────────────────────────────────────────── */
void SmartManager::interestReport(TransactionManager* txnMgr) {
    printSubHeader("月利率收益报表");
    double totalInterest = 0; int cnt = 0;
    txnMgr->data().forEach([&](const Transaction& t) {
        if (t.type == TxnType::INTEREST) { totalInterest += t.amount; ++cnt; }
    });
    char buf[64];
    snprintf(buf, sizeof(buf), "%d 笔", cnt);        printField("利息结算笔数", buf);
    snprintf(buf, sizeof(buf), "%.2f 元", totalInterest); printField("利息总额", buf);
    // 分月统计（简单按年月分组）
    printSubHeader("按月汇总");
    char months[60][8]; int mCnt = 0;
    txnMgr->data().forEach([&](const Transaction& t) {
        if (t.type != TxnType::INTEREST) return;
        char ym[8]; strncpy(ym, t.datetime, 7); ym[7]='\0';
        bool found = false;
        for (int i = 0; i < mCnt; ++i) if (strcmp(months[i], ym)==0) { found=true; break; }
        if (!found && mCnt < 60) strncpy(months[mCnt++], ym, 7);
    });
    for (int i = 0; i < mCnt; ++i) {
        double sum = 0;
        txnMgr->data().forEach([&](const Transaction& t) {
            if (t.type == TxnType::INTEREST && strncmp(t.datetime, months[i], 7)==0)
                sum += t.amount;
        });
        char buf2[64]; snprintf(buf2, sizeof(buf2), "%.2f 元", sum);
        printField(months[i], buf2);
    }
}

/* ─── 8.5 身份核验 ──────────────────────────────────────────── */
bool SmartManager::verifyIdentity(const char* customerId, const char* password) {
    Customer* c = cMgr_->findById(customerId);
    if (!c || strcmp(c->password, password) != 0) return false;
    // 生成模拟验证码
    srand((unsigned)time(nullptr));
    int code = 1000 + rand() % 9000;
    char codeStr[16]; snprintf(codeStr, sizeof(codeStr), "%04d", code);
    setColor(Color::LYELLOW);
    printf("\n  [系统模拟] 验证码已发送至 %s: %s\n\n", c->phone, codeStr);
    resetColor();
    char input[16]; getInput("请输入验证码: ", input, sizeof(input));
    return strcmp(input, codeStr) == 0;
}

/* ─── 交互操作 ──────────────────────────────────────────────── */
void SmartManager::doAnomalyReport() {
    reportAnomalies();
    pause();
}

void SmartManager::doCreditScore() {
    printSubHeader("客户信用评分");
    char id[MAX_ID]; getInput("请输入客户编号（空=全部更新）: ", id, MAX_ID);
    if (id[0] == '\0') {
        updateAllCreditScores();
        printSuccess("所有客户信用分已更新");
    } else {
        Customer* c = cMgr_->findById(id);
        if (!c) { printError("客户不存在"); pause(); return; }
        double score = calcCreditScore(*c, cardMgr_, txnMgr_);
        c->creditScore = score;
        cMgr_->save();
        char buf[64]; snprintf(buf, sizeof(buf), "信用评分: %.1f", score);
        printSuccess(buf);
        // 评级描述
        const char* level;
        if      (score >= 800) level = "★★★★★ 优秀";
        else if (score >= 700) level = "★★★★  良好";
        else if (score >= 600) level = "★★★   中等";
        else if (score >= 400) level = "★★    一般";
        else                   level = "★     差";
        printField("信用等级", level);
    }
    pause();
}

void SmartManager::doLoanApproval() {
    printSubHeader("贷款风控审批");
    char id[MAX_ID]; getInput("客户编号: ", id, MAX_ID);
    double amount = getDoubleInput("申请贷款金额(元): ", 0.01, 10000000.0);
    char reason[256];
    bool passed = loanApproval(id, amount, reason, sizeof(reason));
    if (passed) printSuccess(reason);
    else        printError(reason);
    pause();
}

void SmartManager::doStatistics() {
    customerStatistics();
    printf("\n");
    assetDistribution();
    pause();
}

void SmartManager::doInterestReport() {
    interestReport(txnMgr_);
    pause();
}

void SmartManager::doIdentityVerify() {
    printSubHeader("身份核验（密码 + 验证码）");
    char id[MAX_ID]; getInput("客户编号: ", id, MAX_ID);
    char pwd[MAX_PASS]; getPassword("登录密码: ", pwd, MAX_PASS);
    if (verifyIdentity(id, pwd)) printSuccess("身份核验通过！");
    else                         printError("身份核验失败（密码或验证码错误）");
    pause();
}

/* ─── 菜单入口 ──────────────────────────────────────────────── */
void SmartManager::menuAdmin() {
    const char* items[] = {
        "交易异常报告", "客户信用评分管理",
        "贷款风控审批", "客户统计分析",
        "月利率报表"
    };
    while (true) {
        printHeader("智能管理 - 管理员");
        int c = showMenu("请选择功能", items, 5);
        if      (c == 1) doAnomalyReport();
        else if (c == 2) doCreditScore();
        else if (c == 3) doLoanApproval();
        else if (c == 4) doStatistics();
        else if (c == 5) doInterestReport();
        else             return;
    }
}

void SmartManager::menuEmployee() {
    const char* items[] = {
        "交易异常报告", "贷款风控审批",
        "客户身份核验", "客户统计分析"
    };
    while (true) {
        printHeader("智能管理 - 职员");
        int c = showMenu("请选择功能", items, 4);
        if      (c == 1) doAnomalyReport();
        else if (c == 2) doLoanApproval();
        else if (c == 3) doIdentityVerify();
        else if (c == 4) doStatistics();
        else             return;
    }
}
