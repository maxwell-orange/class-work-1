#pragma once
#include "types.h"
#include "datastructs.h"

/*=============================================================
 *  模块1：银行职员管理
 *  - 管理员：增删改查职员
 *  - 职员：修改自身密码和信息
 *=============================================================*/

#define EMPLOYEE_FILE  "data\\employees.dat"

class EmployeeManager {
public:
    EmployeeManager();
    ~EmployeeManager();

    // ── 登录 ──────────────────────────────────────────────
    // 返回 nullptr 表示失败
    Employee* login(const char* id, const char* password);

    // ── 管理员操作 ────────────────────────────────────────
    bool addEmployee(const Employee& e);
    bool deleteEmployee(const char* id);
    bool updateEmployee(const Employee& e);  // 通过 id 查找并替换
    Employee* findById(const char* id);

    // ── 查询 ─────────────────────────────────────────────
    int queryByName(const char* name, Employee* out, int maxOut);
    int queryByDept(const char* dept, Employee* out, int maxOut);
    void listAll();

    // ── 职员自助 ──────────────────────────────────────────
    bool changePassword(const char* id, const char* oldPwd, const char* newPwd);

    // ── 持久化 ────────────────────────────────────────────
    void save();
    void load();

    // ── UI 入口 ───────────────────────────────────────────
    void menuAdmin();     // 管理员菜单
    void menuEmployee();  // 职员自助菜单

private:
    LinkedList<Employee> list_;
    void printEmployeeDetail(const Employee& e);
    void printEmployeeTable(const Employee* arr, int cnt);
    void doAdd();
    void doDelete();
    void doUpdate();
    void doQuery();
    void doList();
    void doChangePwd(const char* selfId);
    void doUpdateSelf(const char* selfId);
};
