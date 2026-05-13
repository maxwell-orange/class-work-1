#pragma once
#include "types.h"
#include "datastructs.h"

/*=============================================================
 *  模块2：客户账户管理
 *  - 职员：增删改查客户
 *  - 客户：查看/修改自身信息、密码
 *  - 广义表展示客户-卡关联结构
 *=============================================================*/

#define CUSTOMER_FILE "data\\customers.dat"

class BankCardManager; // 前向声明

class CustomerManager {
public:
    CustomerManager();
    ~CustomerManager();

    // ── 核心 CRUD ──────────────────────────────────────────
    bool addCustomer(const Customer& c);
    bool deleteCustomer(const char* id);
    bool updateCustomer(const Customer& c);
    Customer* findById(const char* id);
    Customer* loginCustomer(const char* id, const char* password);

    int queryByName(const char* name, Customer* out, int maxOut);
    int queryByType(CustType t,       Customer* out, int maxOut);
    int queryByPhone(const char* phone, Customer* out, int maxOut);
    void listAll();

    bool changePassword(const char* id, const char* oldPwd, const char* newPwd);
    // 更新持卡数（银行卡模块调用）
    void updateCardCount(const char* customerId, int delta);

    // ── 持久化 ────────────────────────────────────────────
    void save();
    void load();

    // ── UI 入口 ───────────────────────────────────────────
    void menuEmployee(BankCardManager* cardMgr = nullptr);
    void menuCustomer();

    LinkedList<Customer>& data() { return list_; }

private:
    LinkedList<Customer> list_;

    void printCustomerDetail(const Customer& c);
    void printCustomerTable(const Customer* arr, int cnt);
    void showGList(const Customer& c, BankCardManager* cardMgr);

    void doAdd();
    void doDelete();
    void doUpdate();
    void doQuery(BankCardManager* cardMgr);
    void doList();
    void doChangePwd(const char* selfId);
    void doUpdateSelf(const char* selfId);
};
