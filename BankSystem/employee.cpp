#include "employee.h"
#include "fileio.h"
#include "ui.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

EmployeeManager::EmployeeManager() { load(); }
EmployeeManager::~EmployeeManager() { save(); }

/* ─── 登录 ─────────────────────────────────────────────────── */
Employee* EmployeeManager::login(const char* id, const char* password) {
    return list_.find_if([&](const Employee& e) {
        return e.isActive && strcmp(e.id, id) == 0 && strcmp(e.password, password) == 0;
    });
}

/* ─── 增删改查 ─────────────────────────────────────────────── */
bool EmployeeManager::addEmployee(const Employee& e) {
    if (findById(e.id)) return false;  // ID 已存在
    list_.push_back(e);
    save();
    return true;
}

bool EmployeeManager::deleteEmployee(const char* id) {
    // 软删除：将 isActive 置 false
    Employee* e = findById(id);
    if (!e || e->isAdmin) return false;
    e->isActive = false;
    save();
    return true;
}

bool EmployeeManager::updateEmployee(const Employee& e) {
    Employee* found = findById(e.id);
    if (!found) return false;
    // 保留密码不变（不允许通过此接口修改密码）
    char pwd[MAX_PASS];
    strncpy(pwd, found->password, MAX_PASS - 1);
    *found = e;
    strncpy(found->password, pwd, MAX_PASS - 1);
    save();
    return true;
}

Employee* EmployeeManager::findById(const char* id) {
    return list_.find_if([&](const Employee& e) {
        return e.isActive && strcmp(e.id, id) == 0;
    });
}

int EmployeeManager::queryByName(const char* name, Employee* out, int maxOut) {
    return list_.collect([&](const Employee& e) {
        return e.isActive && strstr(e.name, name) != nullptr;
    }, out, maxOut);
}

int EmployeeManager::queryByDept(const char* dept, Employee* out, int maxOut) {
    return list_.collect([&](const Employee& e) {
        return e.isActive && strstr(e.department, dept) != nullptr;
    }, out, maxOut);
}

bool EmployeeManager::changePassword(const char* id, const char* oldPwd, const char* newPwd) {
    Employee* e = list_.find_if([&](const Employee& e) {
        return e.isActive && strcmp(e.id, id) == 0 && strcmp(e.password, oldPwd) == 0;
    });
    if (!e) return false;
    strncpy(e->password, newPwd, MAX_PASS - 1);
    save();
    return true;
}

/* ─── 持久化 ───────────────────────────────────────────────── */
void EmployeeManager::save() {
    saveList(EMPLOYEE_FILE, list_);
}
void EmployeeManager::load() {
    loadList(EMPLOYEE_FILE, list_);
    // 若无数据，创建默认管理员账号
    if (list_.size() == 0) {
        Employee admin;
        strncpy(admin.id,         "admin",    MAX_ID   - 1);
        strncpy(admin.name,       "系统管理员", MAX_NAME - 1);
        strncpy(admin.password,   "admin123", MAX_PASS - 1);
        strncpy(admin.phone,      "10086",    MAX_PHONE - 1);
        strncpy(admin.department, "管理部",   MAX_DEPT  - 1);
        strncpy(admin.position,   "管理员",   MAX_NAME  - 1);
        getCurrentDate(admin.joinDate, MAX_DATE);
        admin.isAdmin  = true;
        admin.isActive = true;
        list_.push_back(admin);
        save();
    }
}

/* ─── 打印函数 ─────────────────────────────────────────────── */
void EmployeeManager::printEmployeeDetail(const Employee& e) {
    printLine('-');
    printField("职员编号", e.id);
    printField("姓    名", e.name);
    printField("部    门", e.department);
    printField("职    位", e.position);
    printField("联系电话", e.phone);
    printField("入职日期", e.joinDate);
    printField("账号类型", e.isAdmin ? "管理员" : "普通职员");
}

void EmployeeManager::printEmployeeTable(const Employee* arr, int cnt) {
    const char* cols[] = {"职员编号", "姓名", "部门", "职位", "电话", "类型"};
    int widths[] = {12, 12, 12, 12, 16, 8};
    printTableHeader(cols, widths, 6);
    printTableSep(widths, 6);
    for (int i = 0; i < cnt; ++i) {
        const char* vals[] = {
            arr[i].id, arr[i].name, arr[i].department,
            arr[i].position, arr[i].phone,
            arr[i].isAdmin ? "管理员" : "职员"
        };
        printTableRow(vals, widths, 6);
    }
    printTableSep(widths, 6);
}

void EmployeeManager::listAll() {
    Employee arr[256]; int cnt = 0;
    list_.forEach([&](const Employee& e) {
        if (e.isActive && cnt < 256) arr[cnt++] = e;
    });
    printEmployeeTable(arr, cnt);
}

/* ─── 交互式操作 ───────────────────────────────────────────── */
void EmployeeManager::doAdd() {
    printSubHeader("添加职员");
    Employee e;
    getInput("职员编号（唯一）: ", e.id, MAX_ID);
    if (findById(e.id)) { printError("该编号已存在！"); pause(); return; }
    getInput("姓          名: ",  e.name,       MAX_NAME);
    getPassword("初 始 密 码: ", e.password, MAX_PASS);
    getInput("联 系 电 话: ",     e.phone,      MAX_PHONE);
    getInput("所 属 部 门: ",     e.department, MAX_DEPT);
    getInput("职          位: ",  e.position,   MAX_NAME);
    getCurrentDate(e.joinDate, MAX_DATE);
    char isAdminStr[8]; getInput("是否管理员(y/n): ", isAdminStr, sizeof(isAdminStr));
    e.isAdmin  = (isAdminStr[0] == 'y' || isAdminStr[0] == 'Y');
    e.isActive = true;
    if (addEmployee(e)) printSuccess("职员添加成功！");
    else                printError("添加失败！");
    pause();
}

void EmployeeManager::doDelete() {
    printSubHeader("删除职员");
    char id[MAX_ID]; getInput("请输入职员编号: ", id, MAX_ID);
    Employee* e = findById(id);
    if (!e) { printError("职员不存在！"); pause(); return; }
    printEmployeeDetail(*e);
    if (!confirm("确认删除此职员？")) { printInfo("已取消"); pause(); return; }
    if (deleteEmployee(id)) printSuccess("删除成功（已标记为离职）");
    else                    printError("删除失败（不能删除管理员）");
    pause();
}

void EmployeeManager::doUpdate() {
    printSubHeader("修改职员信息");
    char id[MAX_ID]; getInput("请输入职员编号: ", id, MAX_ID);
    Employee* e = findById(id);
    if (!e) { printError("职员不存在！"); pause(); return; }
    printEmployeeDetail(*e);
    printInfo("直接回车跳过该字段不修改");
    Employee ne = *e;
    char buf[MAX_NAME];
    getInput("新姓名: ",   buf, MAX_NAME);  if (buf[0]) strncpy(ne.name,       buf, MAX_NAME-1);
    getInput("新电话: ",   buf, MAX_PHONE); if (buf[0]) strncpy(ne.phone,      buf, MAX_PHONE-1);
    getInput("新部门: ",   buf, MAX_DEPT);  if (buf[0]) strncpy(ne.department, buf, MAX_DEPT-1);
    getInput("新职位: ",   buf, MAX_NAME);  if (buf[0]) strncpy(ne.position,   buf, MAX_NAME-1);
    if (updateEmployee(ne)) printSuccess("修改成功！");
    else                    printError("修改失败！");
    pause();
}

void EmployeeManager::doQuery() {
    printSubHeader("查询职员");
    const char* opts[] = {"按编号查询", "按姓名查询", "按部门查询"};
    int choice = showMenu("查询方式", opts, 3);
    Employee arr[64]; int cnt = 0;
    char buf[MAX_NAME];
    if (choice == 1) {
        getInput("请输入编号: ", buf, MAX_ID);
        Employee* e = findById(buf);
        if (e) { arr[0]=*e; cnt=1; }
    } else if (choice == 2) {
        getInput("请输入姓名关键字: ", buf, MAX_NAME);
        cnt = queryByName(buf, arr, 64);
    } else if (choice == 3) {
        getInput("请输入部门名称: ", buf, MAX_DEPT);
        cnt = queryByDept(buf, arr, 64);
    } else return;
    if (cnt == 0) { printWarning("未找到匹配职员"); }
    else {
        char numBuf[32]; snprintf(numBuf, sizeof(numBuf), "共找到 %d 名职员", cnt);
        printInfo(numBuf);
        printEmployeeTable(arr, cnt);
    }
    pause();
}

void EmployeeManager::doList() {
    printSubHeader("全部职员列表");
    listAll();
    char buf[32]; snprintf(buf, sizeof(buf), "共 %d 名在职职员", list_.size());
    printInfo(buf);
    pause();
}

void EmployeeManager::doChangePwd(const char* selfId) {
    printSubHeader("修改登录密码");
    char old[MAX_PASS], np[MAX_PASS], np2[MAX_PASS];
    getPassword("当前密码: ",   old, MAX_PASS);
    getPassword("新密码:   ",   np,  MAX_PASS);
    getPassword("确认新密码: ", np2, MAX_PASS);
    if (strcmp(np, np2) != 0) { printError("两次密码不一致！"); pause(); return; }
    if (strlen(np) < 6)       { printError("密码长度不能少于6位！"); pause(); return; }
    if (changePassword(selfId, old, np)) printSuccess("密码修改成功！");
    else                                 printError("原密码错误！");
    pause();
}

void EmployeeManager::doUpdateSelf(const char* selfId) {
    printSubHeader("修改个人信息");
    Employee* e = findById(selfId);
    if (!e) { printError("账号异常"); pause(); return; }
    printEmployeeDetail(*e);
    printInfo("直接回车跳过该字段不修改");
    char buf[MAX_NAME];
    getInput("新电话: ", buf, MAX_PHONE);
    if (buf[0]) { strncpy(e->phone, buf, MAX_PHONE-1); save(); printSuccess("已更新！"); }
    else printInfo("未做修改");
    pause();
}

/* ─── UI 菜单入口 ──────────────────────────────────────────── */
void EmployeeManager::menuAdmin() {
    const char* items[] = {
        "添加职员", "删除职员", "修改职员信息",
        "查询职员", "显示所有职员"
    };
    while (true) {
        printHeader("职员管理 - 管理员");
        int c = showMenu("请选择操作", items, 5);
        if      (c == 1) doAdd();
        else if (c == 2) doDelete();
        else if (c == 3) doUpdate();
        else if (c == 4) doQuery();
        else if (c == 5) doList();
        else             return;
    }
}

void EmployeeManager::menuEmployee() {
    const char* items[] = {"修改登录密码", "修改个人信息", "查看个人信息"};
    while (true) {
        printHeader("职员自助服务");
        extern Session g_session;
        int c = showMenu("请选择操作", items, 3);
        if (c == 1)      doChangePwd(g_session.userId);
        else if (c == 2) doUpdateSelf(g_session.userId);
        else if (c == 3) {
            Employee* e = findById(g_session.userId);
            if (e) printEmployeeDetail(*e);
            pause();
        }
        else return;
    }
}
