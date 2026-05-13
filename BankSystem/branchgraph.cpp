#include "branchgraph.h"
#include "fileio.h"
#include "ui.h"
#include <cstring>
#include <cstdio>
#include <cmath>

BranchGraphManager::BranchGraphManager() { load(); }
BranchGraphManager::~BranchGraphManager() { save(); }

/* ─── 网点 CRUD ──────────────────────────────────────────────── */
bool BranchGraphManager::addBranch(const Branch& b) {
    if (findBranch(b.id)) return false;
    branches_.push_back(b);
    graph_.addVertex(b.id, b.name);
    save();
    return true;
}

bool BranchGraphManager::deleteBranch(const char* id) {
    Branch* b = findBranch(id);
    if (!b) return false;
    b->isActive = false;
    graph_.removeVertex(id);
    save();
    return true;
}

Branch* BranchGraphManager::findBranch(const char* id) {
    return branches_.find_if([&](const Branch& b) {
        return b.isActive && strcmp(b.id, id) == 0;
    });
}

void BranchGraphManager::addRoad(const char* fromId, const char* toId, double distKm) {
    graph_.addEdge(fromId, toId, distKm);
    save();
}

void BranchGraphManager::removeRoad(const char* fromId, const char* toId) {
    graph_.removeEdge(fromId, toId);
    save();
}

bool BranchGraphManager::shortestPath(const char* fromId, const char* toId,
                                       char* pathBuf, int bufLen, double& dist) {
    return graph_.shortestPath(fromId, toId, pathBuf, bufLen, dist);
}

void BranchGraphManager::showAllReachable(const char* fromId) {
    char names[Graph::MAXV][52];
    int cnt = graph_.reachable(fromId, names, Graph::MAXV);
    if (cnt == 0) { printWarning("从该网点无法到达其他网点"); return; }
    char buf[48]; snprintf(buf, sizeof(buf), "从 %s 可达 %d 个网点:", fromId, cnt);
    printInfo(buf);
    for (int i = 0; i < cnt; ++i) {
        printf("    -> %s\n", names[i]);
    }
}

/* ─── 打印 ───────────────────────────────────────────────────── */
void BranchGraphManager::printBranchDetail(const Branch& b) {
    printLine('-');
    printField("网点编号",  b.id);
    printField("网点名称",  b.name);
    printField("地    址",  b.address);
    printField("联系电话",  b.phone);
    char pos[64]; snprintf(pos, sizeof(pos), "(%.2f, %.2f) km", b.x, b.y);
    printField("坐    标",  pos);
}

void BranchGraphManager::printBranchTable(const Branch* arr, int cnt) {
    const char* cols[] = {"编号", "名称", "地址", "电话"};
    int widths[] = {8, 18, 30, 16};
    printTableHeader(cols, widths, 4);
    printTableSep(widths, 4);
    for (int i = 0; i < cnt; ++i) {
        const char* vals[] = {arr[i].id, arr[i].name, arr[i].address, arr[i].phone};
        printTableRow(vals, widths, 4);
    }
    printTableSep(widths, 4);
}

void BranchGraphManager::listBranches() {
    Branch arr[Graph::MAXV]; int cnt = 0;
    branches_.forEach([&](const Branch& b) {
        if (b.isActive && cnt < Graph::MAXV) arr[cnt++] = b;
    });
    printBranchTable(arr, cnt);
}

/* ─── ASCII 地图 ─────────────────────────────────────────────── */
void BranchGraphManager::displayMap() {
    printSubHeader("网点拓扑地图");
    // 简单文字地图：列出网点及其相邻节点
    setColor(Color::LCYAN);
    printf("\n  网点连通关系图（邻接表形式）:\n\n");
    resetColor();
    for (int i = 0; i < graph_.vCnt; ++i) {
        if (!graph_.verts[i].valid) continue;
        setColor(Color::LYELLOW);
        printf("  %-20s ──> ", graph_.verts[i].name);
        setColor(Color::LWHITE);
        bool any = false;
        for (int j = 0; j < graph_.vCnt; ++j) {
            if (!graph_.verts[j].valid || i == j) continue;
            if (graph_.dist[i][j] < (double)GRAPH_INF) {
                printf("%s(%.1fkm)  ", graph_.verts[j].name, graph_.dist[i][j]);
                any = true;
            }
        }
        if (!any) printf("（孤立节点）");
        printf("\n");
        resetColor();
    }
    printf("\n");
    resetColor();
}

/* ─── 预置示例数据 ───────────────────────────────────────────── */
void BranchGraphManager::initSampleData() {
    struct BranchInfo { const char* id; const char* name; const char* addr;
                        const char* phone; double x; double y; };
    BranchInfo infos[] = {
        {"B001","总行营业部","北京市朝阳区建国路 1 号","010-12345001",0,0},
        {"B002","朝阳支行", "北京市朝阳区朝阳门外大街 22 号","010-12345002",3,2},
        {"B003","海淀支行", "北京市海淀区中关村大街 16 号","010-12345003",-5,4},
        {"B004","西城支行", "北京市西城区长安街 8 号",   "010-12345004",-3,-1},
        {"B005","东城支行", "北京市东城区东直门内大街 5 号","010-12345005",5,-2},
        {"B006","通州支行", "北京市通州区运河东大街 66 号","010-12345006",12,1},
    };
    for (auto& info : infos) {
        Branch b;
        strncpy(b.id,      info.id,    MAX_ID-1);
        strncpy(b.name,    info.name,  MAX_NAME-1);
        strncpy(b.address, info.addr,  MAX_ADDR-1);
        strncpy(b.phone,   info.phone, MAX_PHONE-1);
        b.x = info.x; b.y = info.y; b.isActive = true;
        addBranch(b);
    }
    // 添加道路（根据坐标距离近似）
    addRoad("B001","B002",3.6);
    addRoad("B001","B004",3.2);
    addRoad("B001","B005",5.1);
    addRoad("B002","B006",9.0);
    addRoad("B003","B004",2.8);
    addRoad("B004","B001",3.2);
    addRoad("B005","B006",7.2);
    addRoad("B003","B001",5.9);
    save();
}

/* ─── 持久化 ─────────────────────────────────────────────────── */
void BranchGraphManager::save() {
    saveList(BRANCH_FILE, branches_);
    saveGraph(GRAPH_FILE, graph_);
}

void BranchGraphManager::load() {
    if (!loadList(BRANCH_FILE, branches_) || branches_.size() == 0) {
        initSampleData();
    } else {
        loadGraph(GRAPH_FILE, graph_);
    }
}

/* ─── 交互操作 ───────────────────────────────────────────────── */
void BranchGraphManager::doAddBranch() {
    printSubHeader("添加网点");
    Branch b;
    getInput("网点编号: ", b.id,      MAX_ID);
    if (findBranch(b.id)) { printError("编号已存在"); pause(); return; }
    getInput("网点名称: ", b.name,    MAX_NAME);
    getInput("地    址: ", b.address, MAX_ADDR);
    getInput("联系电话: ", b.phone,   MAX_PHONE);
    b.x = getDoubleInput("X坐标(km): ", -1000, 1000);
    b.y = getDoubleInput("Y坐标(km): ", -1000, 1000);
    b.isActive = true;
    if (addBranch(b)) printSuccess("网点添加成功！");
    else              printError("添加失败！");
    pause();
}

void BranchGraphManager::doDeleteBranch() {
    printSubHeader("删除网点");
    listBranches();
    char id[MAX_ID]; getInput("请输入网点编号: ", id, MAX_ID);
    Branch* b = findBranch(id);
    if (!b) { printError("网点不存在"); pause(); return; }
    if (!confirm("确认删除该网点及相关路段？")) { printInfo("已取消"); pause(); return; }
    if (deleteBranch(id)) printSuccess("已删除网点");
    else                  printError("删除失败");
    pause();
}

void BranchGraphManager::doAddRoad() {
    printSubHeader("添加/修改路段");
    listBranches();
    char from[MAX_ID], to[MAX_ID];
    getInput("起始网点编号: ", from, MAX_ID);
    getInput("目的网点编号: ", to,   MAX_ID);
    double d = getDoubleInput("距离(km): ", 0.1, 9999);
    addRoad(from, to, d);
    printSuccess("路段已添加");
    pause();
}

void BranchGraphManager::doShortestPath() {
    printSubHeader("最短路径导航");
    listBranches();
    char from[MAX_ID], to[MAX_ID];
    getInput("出发网点编号: ", from, MAX_ID);
    getInput("目标网点编号: ", to,   MAX_ID);
    char pathBuf[512]; double dist;
    if (shortestPath(from, to, pathBuf, sizeof(pathBuf), dist)) {
        char buf[64]; snprintf(buf, sizeof(buf), "最短距离: %.2f km", dist);
        printSuccess(buf);
        setColor(Color::LGREEN);
        printf("  路径: %s\n", pathBuf);
        resetColor();
    } else {
        printError("两网点之间不可达");
    }
    pause();
}

void BranchGraphManager::doReachable() {
    printSubHeader("可达网点查询");
    listBranches();
    char from[MAX_ID]; getInput("出发网点编号: ", from, MAX_ID);
    showAllReachable(from);
    pause();
}

/* ─── 菜单入口 ───────────────────────────────────────────────── */
void BranchGraphManager::menuAdmin() {
    const char* items[] = {
        "添加网点", "删除网点", "添加/修改路段",
        "查询所有网点", "显示拓扑地图"
    };
    while (true) {
        printHeader("网点管理 - 管理员");
        int c = showMenu("请选择操作", items, 5);
        if      (c == 1) doAddBranch();
        else if (c == 2) doDeleteBranch();
        else if (c == 3) doAddRoad();
        else if (c == 4) { printSubHeader("网点列表"); listBranches(); pause(); }
        else if (c == 5) { displayMap(); pause(); }
        else             return;
    }
}

void BranchGraphManager::menuQuery() {
    const char* items[] = {
        "查询所有网点", "网点路径导航（最短路径）",
        "查询可达网点（BFS）", "显示拓扑地图"
    };
    while (true) {
        printHeader("网点查询导航");
        int c = showMenu("请选择操作", items, 4);
        if      (c == 1) { printSubHeader("网点列表"); listBranches(); pause(); }
        else if (c == 2) doShortestPath();
        else if (c == 3) doReachable();
        else if (c == 4) { displayMap(); pause(); }
        else             return;
    }
}
