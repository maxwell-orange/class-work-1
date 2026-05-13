#pragma once
#include "types.h"
#include "datastructs.h"

/*=============================================================
 *  模块3：银行卡管理
 *  - 职员：增删改查银行卡
 *  - 卡号格式：6222XXXXXXXXXXXX（16位）
 *=============================================================*/

#define CARD_FILE "data\\cards.dat"

class BankCardManager {
public:
    BankCardManager();
    ~BankCardManager();

    bool addCard(const BankCard& c);
    bool deleteCard(const char* cardId);
    bool updateCard(const BankCard& c);
    bool freezeCard(const char* cardId, bool freeze);
    BankCard* findByCardId(const char* cardId);

    int getCardsByCustomer(const char* customerId, BankCard* out, int maxOut);
    int queryByType(CardType t, BankCard* out, int maxOut);
    void listAll();

    // 内部余额更新（事务模块调用）
    bool updateBalance(const char* cardId, double delta);
    bool updateLoanBalance(const char* cardId, double delta);

    void save();
    void load();

    void menuEmployee();
    void menuCustomer();

    LinkedList<BankCard>& data() { return list_; }

private:
    LinkedList<BankCard> list_;

    void printCardDetail(const BankCard& c);
    void printCardTable(const BankCard* arr, int cnt);
    static void generateCardId(char* buf);

    void doAdd();
    void doDelete();
    void doUpdate();
    void doQuery();
    void doList();
    void doFreeze();
};
