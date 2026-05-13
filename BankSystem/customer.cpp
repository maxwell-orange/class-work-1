#include "customer.h"
#include "bankcard.h"
#include "fileio.h"
#include "ui.h"
#include <cstring>
#include <cstdio>

CustomerManager::CustomerManager() { load(); }
CustomerManager::~CustomerManager() { save(); }

bool CustomerManager::addCustomer(const Customer& c) {
    if (findById(c.id)) return false;
    list_.push_back(c);
    save();
    return true;
}

bool CustomerManager::deleteCustomer(const char* id) {
    Customer* c = findById(id);
    if (!c) return false;
    c->isActive = false;
    save();
    return true;
}

bool CustomerManager::updateCustomer(const Customer& c) {
    Customer* found = findById(c.id);
    if (!found) return false;
    char pwd[MAX_PASS];
    strncpy(pwd, found->password, MAX_PASS - 1);
    *found = c;
    strncpy(found->password, pwd, MAX_PASS - 1);
    save();
    return true;
}

Customer* CustomerManager::findById(const char* id) {
    return list_.find_if([&](const Customer& c) {
        return c.isActive && strcmp(c.id, id) == 0;
    });
}

Customer* CustomerManager::loginCustomer(const char* id, const char* password) {
    return list_.find_if([&](const Customer& c) {
        return c.isActive && strcmp(c.id, id) == 0 && strcmp(c.password, password) == 0;
    });
}

int CustomerManager::queryByName(const char* name, Customer* out, int maxOut) {
    return list_.collect([&](const Customer& c) {
        return c.isActive && strstr(c.name, name) != nullptr;
    }, out, maxOut);
}

int CustomerManager::queryByType(CustType t, Customer* out, int maxOut) {
    return list_.collect([&](const Customer& c) {
        return c.isActive && c.type == t;
    }, out, maxOut);
}

int CustomerManager::queryByPhone(const char* phone, Customer* out, int maxOut) {
    return list_.collect([&](const Customer& c) {
        return c.isActive && strstr(c.phone, phone) != nullptr;
    }, out, maxOut);
}

void CustomerManager::updateCardCount(const char* customerId, int delta) {
    Customer* c = findById(customerId);
    if (c) { c->cardCount += delta; if (c->cardCount < 0) c->cardCount = 0; save(); }
}

bool CustomerManager::changePassword(const char* id, const char* oldPwd, const char* newPwd) {
    Customer* c = list_.find_if([&](const Customer& c) {
        return c.isActive && strcmp(c.id, id) == 0 && strcmp(c.password, oldPwd) == 0;
    });
    if (!c) return false;
    strncpy(c->password, newPwd, MAX_PASS - 1);
    save();
    return true;
}

void CustomerManager::save() { saveList(CUSTOMER_FILE, list_); }
void CustomerManager::load() { loadList(CUSTOMER_FILE, list_); }

/* ─── 打印 ─────────────────────────────────────────────────── */
void CustomerManager::printCustomerDetail(const Customer& c) {
    printLine('-');
    printField("客户编号",   c.id);
    printField("姓    名",   c.name);
    printField("客户类型",   custTypeName(c.type));
    printField("联系电话",   c.phone);
    printField("通信地址",   c.address);
    printField("开户日期",   c.openDate);
    printFieldF("信用评分",  c.creditScore, 1);
    printFieldF("金融资产",  c.financialAssets, 2);
    printFieldF("月收入",    c.monthlyIncome, 2);
    printFieldF("月支出",    c.monthlyExpenses, 2);
    printFieldI("持卡数量",  c.cardCount);
}

void CustomerManager::printCustomerTable(const Customer* arr, int cnt) {
    const char* cols[] = {"客户编号", "姓名", "类型", "电话", "信用分", "持卡数"};
    int widths[] = {14, 12, 6, 16, 8, 6};
    printTableHeader(cols, widths, 6);
    printTableSep(widths, 6);
    char scoreStr[16], cardStr[8];
    for (int i = 0; i < cnt; ++i) {
        snprintf(scoreStr, sizeof(scoreStr), "%.1f", arr[i].creditScore);
        snprintf(cardStr,  sizeof(cardStr),  "%d",   arr[i].cardCount);
        const char* vals[] = {
            arr[i].id, arr[i].name, custTypeName(arr[i].type),
            arr[i].phone, scoreStr, cardStr
        };
        printTableRow(vals, widths, 6);
    }
    printTableSep(widths, 6);
}

void CustomerManager::listAll() {
    Customer arr[512]; int cnt = 0;
    list_.forEach([&](const Customer& c) {
        if (c.isActive && cnt < 512) arr[cnt++] = c;
    });
    printCustomerTable(arr, cnt);
    char buf[32]; snprintf(buf, sizeof(buf), "共 %d 名客户", cnt);
    printInfo(buf);
}

/* ─── 广义表展示 客户-银行卡 结构 ─────────────────────────── */
void CustomerManager::showGList(const Customer& c, BankCardManager* cardMgr) {
    printSubHeader("客户账户广义表结构");
    // 广义表示例：(客户ID, 姓名, 类型, (卡号1:类型:余额, 卡号2:类型:余额, ...))
    setColor(Color::LYELLOW);
    printf("  广义表: (");
    setColor(Color::LWHITE);
    printf("%s, %s, %s", c.id, c.name, custTypeName(c.type));
    if (cardMgr) {
        BankCard cards[32]; int cnt = cardMgr->getCardsByCustomer(c.id, cards, 32);
        if (cnt > 0) {
            setColor(Color::LYELLOW);
            printf(", (");
            setColor(Color::LGREEN);
            for (int i = 0; i < cnt; ++i) {
                printf("%s:%s:%.2f", cards[i].cardId, cardTypeName(cards[i].type), cards[i].balance);
                if (i < cnt - 1) printf(", ");
            }
            setColor(Color::LYELLOW);
            printf(")");
        }
    }
    setColor(Color::LYELLOW);
    printf(")\n");
    resetColor();
    pause();
}

/* ─── 交互式操作 ───────────────────────────────────────────── */
void CustomerManager::doAdd() {
    printSubHeader("添加客户");
    Customer c;
    getInput("客户编号（唯一）: ", c.id, MAX_ID);
    if (findById(c.id)) { printError("该编号已存在！"); pause(); return; }
    getInput("姓          名: ", c.name,    MAX_NAME);
    getPassword("初 始 密 码: ", c.password, MAX_PASS);
    getInput("联 系 电 话: ",    c.phone,   MAX_PHONE);
    getInput("通 信 地 址: ",    c.address, MAX_ADDR);
    char typeStr[8]; getInput("客户类型(1普通/2VIP): ", typeStr, sizeof(typeStr));
    c.type = (typeStr[0]=='2') ? CustType::VIP : CustType::NORMAL;
    c.creditScore    = getDoubleInput("初始信用分(0-1000): ", 0, 1000);
    c.financialAssets= getDoubleInput("金融资产(元): ", 0, 1e9);
    c.monthlyIncome  = getDoubleInput("月收入(元): ", 0, 1e8);
    c.monthlyExpenses= getDoubleInput("月支出(元): ", 0, 1e8);
    getCurrentDate(c.openDate, MAX_DATE);
    c.isActive = true;
    if (addCustomer(c)) printSuccess("客户添加成功！");
    else                printError("添加失败！");
    pause();
}

void CustomerManager::doDelete() {
    printSubHeader("删除客户");
    char id[MAX_ID]; getInput("请输入客户编号: ", id, MAX_ID);
    Customer* c = findById(id);
    if (!c) { printError("客户不存在！"); pause(); return; }
    printCustomerDetail(*c);
    if (!confirm("确认注销此客户？")) { printInfo("已取消"); pause(); return; }
    if (deleteCustomer(id)) printSuccess("已注销客户账户");
    else                    printError("注销失败");
    pause();
}

void CustomerManager::doUpdate() {
    printSubHeader("修改客户信息");
    char id[MAX_ID]; getInput("请输入客户编号: ", id, MAX_ID);
    Customer* c = findById(id);
    if (!c) { printError("客户不存在！"); pause(); return; }
    printCustomerDetail(*c);
    printInfo("直接回车跳过不修改");
    Customer nc = *c;
    char buf[MAX_ADDR];
    getInput("新姓名: ",  buf, MAX_NAME);  if(buf[0]) strncpy(nc.name,    buf, MAX_NAME-1);
    getInput("新电话: ",  buf, MAX_PHONE); if(buf[0]) strncpy(nc.phone,   buf, MAX_PHONE-1);
    getInput("新地址: ",  buf, MAX_ADDR);  if(buf[0]) strncpy(nc.address, buf, MAX_ADDR-1);
    getInput("客户类型(1普通/2VIP): ", buf, 8);
    if(buf[0]) nc.type = (buf[0]=='2') ? CustType::VIP : CustType::NORMAL;
    getInput("月收入(空跳过): ", buf, 32);
    if(buf[0]) sscanf(buf, "%lf", &nc.monthlyIncome);
    getInput("月支出(空跳过): ", buf, 32);
    if(buf[0]) sscanf(buf, "%lf", &nc.monthlyExpenses);
    getInput("金融资产(空跳过): ", buf, 32);
    if(buf[0]) sscanf(buf, "%lf", &nc.financialAssets);
    if (updateCustomer(nc)) printSuccess("修改成功！");
    else                    printError("修改失败！");
    pause();
}

void CustomerManager::doQuery(BankCardManager* cardMgr) {
    printSubHeader("查询客户");
    const char* opts[] = {"按编号查询", "按姓名查询", "按电话查询", "按类型查询(全部VIP/普通)"};
    int choice = showMenu("查询方式", opts, 4);
    Customer arr[128]; int cnt = 0;
    char buf[MAX_NAME];
    if (choice == 1) {
        getInput("请输入编号: ", buf, MAX_ID);
        Customer* c = findById(buf);
        if (c) { arr[0]=*c; cnt=1; }
    } else if (choice == 2) {
        getInput("请输入姓名关键字: ", buf, MAX_NAME);
        cnt = queryByName(buf, arr, 128);
    } else if (choice == 3) {
        getInput("请输入电话关键字: ", buf, MAX_PHONE);
        cnt = queryByPhone(buf, arr, 128);
    } else if (choice == 4) {
        getInput("类型(1普通/2VIP): ", buf, 8);
        cnt = queryByType(buf[0]=='2' ? CustType::VIP : CustType::NORMAL, arr, 128);
    } else return;

    if (cnt == 0) { printWarning("未找到匹配客户"); pause(); return; }
    char numBuf[32]; snprintf(numBuf, sizeof(numBuf), "共找到 %d 名客户", cnt);
    printInfo(numBuf);
    printCustomerTable(arr, cnt);

    if (cnt == 1 && cardMgr) {
        if (confirm("查看广义表结构？")) showGList(arr[0], cardMgr);
    } else pause();
}

void CustomerManager::doList() {
    printSubHeader("全部客户列表");
    listAll();
    pause();
}

void CustomerManager::doChangePwd(const char* selfId) {
    printSubHeader("修改登录密码");
    char old[MAX_PASS], np[MAX_PASS], np2[MAX_PASS];
    getPassword("当前密码: ",   old, MAX_PASS);
    getPassword("新密码:   ",   np,  MAX_PASS);
    getPassword("确认新密码: ", np2, MAX_PASS);
    if (strcmp(np, np2) != 0)  { printError("两次密码不一致！"); pause(); return; }
    if ((int)strlen(np) < 6)   { printError("密码长度不能少于6位！"); pause(); return; }
    if (changePassword(selfId, old, np)) printSuccess("密码修改成功！");
    else                                 printError("原密码错误！");
    pause();
}

void CustomerManager::doUpdateSelf(const char* selfId) {
    printSubHeader("修改个人信息");
    Customer* c = findById(selfId);
    if (!c) { printError("账号异常"); pause(); return; }
    printCustomerDetail(*c);
    printInfo("直接回车跳过不修改");
    char buf[MAX_ADDR];
    getInput("新电话: ", buf, MAX_PHONE); if(buf[0]) { strncpy(c->phone, buf, MAX_PHONE-1); save(); }
    getInput("新地址: ", buf, MAX_ADDR);  if(buf[0]) { strncpy(c->address, buf, MAX_ADDR-1); save(); }
    printSuccess("个人信息已更新");
    pause();
}

/* ─── 菜单入口 ─────────────────────────────────────────────── */
void CustomerManager::menuEmployee(BankCardManager* cardMgr) {
    const char* items[] = {
        "添加客户", "删除客户", "修改客户信息",
        "查询客户", "显示所有客户"
    };
    while (true) {
        printHeader("客户账户管理");
        int c = showMenu("请选择操作", items, 5);
        if      (c == 1) doAdd();
        else if (c == 2) doDelete();
        else if (c == 3) doUpdate();
        else if (c == 4) doQuery(cardMgr);
        else if (c == 5) doList();
        else             return;
    }
}

void CustomerManager::menuCustomer() {
    extern Session g_session;
    const char* items[] = {"查看个人信息", "修改密码", "修改联系方式"};
    while (true) {
        printHeader("客户自助服务");
        int c = showMenu("请选择操作", items, 3);
        if (c == 1) {
            Customer* cust = findById(g_session.userId);
            if (cust) { printSubHeader("我的信息"); printCustomerDetail(*cust); }
            pause();
        } else if (c == 2) doChangePwd(g_session.userId);
        else if (c == 3)   doUpdateSelf(g_session.userId);
        else               return;
    }
}
