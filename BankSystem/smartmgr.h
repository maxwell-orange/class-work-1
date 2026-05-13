#pragma once
#include "types.h"
#include "datastructs.h"
#include "customer.h"
#include "bankcard.h"
#include "transaction.h"

/*=============================================================
 *  模块8：智能管理（创新功能）
 *  8.1 交易异常报告（大额/频繁交易）
 *  8.2 信用评级与风控审批（贷款评估）
 *  8.3 客户统计分析（收支、资产分布）
 *  8.4 月利率报表
 *  8.5 身份核验（密码 + 验证码模拟）
 *=============================================================*/

class SmartManager {
public:
    SmartManager(CustomerManager* cMgr, BankCardManager* cardMgr,
                 TransactionManager* txnMgr);

    // 8.1 大额/频繁交易检测
    int detectAnomalies(Transaction* out, int maxOut);
    void reportAnomalies();

    // 8.2 信用评级（自动更新）
    double calcCreditScore(const Customer& c, BankCardManager* cardMgr,
                           TransactionManager* txnMgr);
    void updateAllCreditScores();
    // 贷款风控审批
    bool loanApproval(const char* customerId, double amount, char* reason, int reasonLen);

    // 8.3 客户统计分析
    void customerStatistics();
    void assetDistribution();

    // 8.4 月利率报表
    void interestReport(TransactionManager* txnMgr);

    // 8.5 简易身份核验（密码 + 4位数字验证码）
    bool verifyIdentity(const char* customerId, const char* password);

    // UI 入口
    void menuAdmin();
    void menuEmployee();

private:
    CustomerManager*    cMgr_;
    BankCardManager*    cardMgr_;
    TransactionManager* txnMgr_;

    void doAnomalyReport();
    void doCreditScore();
    void doLoanApproval();
    void doStatistics();
    void doInterestReport();
    void doIdentityVerify();
};
