#pragma once
#include "types.h"
#include "datastructs.h"

/*=============================================================
 *  模块7：银行网点查询
 *  - 建立网点地图（图）
 *  - 查询网点信息
 *  - 网点路径导航（最短路径 Dijkstra）
 *  - 可达路径（BFS）
 *  - 网点增删
 *=============================================================*/

#define BRANCH_FILE "data\\branches.dat"
#define GRAPH_FILE  "data\\branch_graph.dat"

class BranchGraphManager {
public:
    BranchGraphManager();
    ~BranchGraphManager();

    // ── 网点管理 ──────────────────────────────────────────
    bool addBranch(const Branch& b);
    bool deleteBranch(const char* id);
    Branch* findBranch(const char* id);
    void listBranches();

    // ── 边（路段）管理 ────────────────────────────────────
    void addRoad(const char* fromId, const char* toId, double distKm);
    void removeRoad(const char* fromId, const char* toId);

    // ── 路径查询 ──────────────────────────────────────────
    bool shortestPath(const char* fromId, const char* toId,
                      char* pathBuf, int bufLen, double& dist);
    void showAllReachable(const char* fromId);

    // ── 地图展示（ASCII） ─────────────────────────────────
    void displayMap();

    // ── 持久化 ────────────────────────────────────────────
    void save();
    void load();

    // ── UI 入口 ───────────────────────────────────────────
    void menuAdmin();
    void menuQuery();

private:
    LinkedList<Branch> branches_;
    Graph graph_;

    void printBranchDetail(const Branch& b);
    void printBranchTable(const Branch* arr, int cnt);
    void initSampleData();   // 预置示例数据
    void doAddBranch();
    void doDeleteBranch();
    void doAddRoad();
    void doShortestPath();
    void doReachable();
};
