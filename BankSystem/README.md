# 银行综合管理系统 v1.0

> Bank Management System — C++ 课程设计作业

## 项目简介

本项目是一个基于 **Windows 控制台**、使用 **C++14** 实现的银行综合管理系统，涵盖职员管理、客户账户、银行卡业务、存贷款、业务查询、排队叫号、网点导航和智能风控等 10 个功能模块，全部数据结构均为手写实现（不依赖 STL 容器），UI 采用 Windows Console API 彩色字符界面。

---

## 功能模块

| 模块 | 名称 | 说明 |
|:---:|------|------|
| 1 | 职员管理 | 职员信息增删改查、权限控制、密码修改 |
| 2 | 客户账户管理 | 客户注册/注销/信息修改、广义表展示客户-卡结构 |
| 3 | 银行卡管理 | 开卡/销卡/冻结、支持借记卡/储蓄卡/信用卡 |
| 4 | 存贷款业务 | 存款、取款、转账、申请贷款、还款 |
| 5 | 业务查询 | 按日期/类型/金额范围/客户类型多维度查询交易记录 |
| 6 | 排队管理 | VIP 优先队列 + 普通队列、叫号、服务评分、每日统计 |
| 7 | 网点查询导航 | 图结构建模、Dijkstra 最短路径、BFS 可达路径、ASCII 地图 |
| 8 | 智能管理 | 异常交易检测、信用评分、贷款风控审批、客户统计分析 |
| 9 | 系统主界面 | 管理员/职员/客户三角色登录、会话管理 |
| 10 | 数据持久化 | 二进制文件序列化，程序重启数据不丢失 |

---

## 自定义数据结构

本项目**不使用** STL 容器（vector/list/queue/stack 等），所有数据结构均在 `datastructs.h` 中手写实现：

| 数据结构 | 实现方式 | 用途 |
|---------|---------|------|
| `LinkedList<T>` | 双向链表，模板类 | 职员/客户/银行卡/交易记录存储 |
| `Stack<T>` | 链式栈，模板类 | 操作历史（保留扩展） |
| `Queue<T>` | 链式队列，模板类 | 普通客户排队队列 |
| `PriorityQueue<T>` | 最大二叉堆，模板类 | VIP 客户优先排队 |
| `Graph` | 邻接矩阵（50×50）| 银行网点路径图 |
| `GList` | 广义表 | 展示客户-银行卡层级结构 |

---

## 目录结构

```
BankSystem/
├── types.h           # 全局常量、枚举、所有数据结构体定义
├── datastructs.h     # 自定义数据结构（header-only 模板）
├── ui.h / ui.cpp     # Windows 控制台彩色 UI 工具函数
├── fileio.h / fileio.cpp   # 二进制文件序列化 I/O
├── employee.h/.cpp   # 模块1：职员管理
├── customer.h/.cpp   # 模块2：客户账户管理
├── bankcard.h/.cpp   # 模块3：银行卡管理
├── transaction.h/.cpp  # 模块4+5：存贷款业务 & 业务查询
├── bankqueue.h/.cpp  # 模块6：排队管理
├── branchgraph.h/.cpp  # 模块7：网点查询导航
├── smartmgr.h/.cpp   # 模块8：智能管理
├── main.cpp          # 模块9：主界面 & 程序入口
├── Makefile          # make 构建脚本
├── CMakeLists.txt    # CMake 构建脚本
└── data/             # 运行时自动创建，存放 .dat 二进制数据文件
```

---

## 编译与运行

### 环境要求

- **编译器**：g++ 8.1.0 及以上（MinGW-W64），支持 C++14
- **操作系统**：Windows 7 / 10 / 11（需要 Windows Console API）

### 使用 Makefile 编译

```bash
cd BankSystem
make         # 编译生成 BankSystem.exe
make run     # 编译并运行
make clean   # 清理编译产物
```

### 手动编译

```bash
g++ -std=c++14 -O2 -o BankSystem.exe \
    main.cpp ui.cpp fileio.cpp employee.cpp customer.cpp bankcard.cpp \
    transaction.cpp bankqueue.cpp branchgraph.cpp smartmgr.cpp
```

### 运行

```bash
cd BankSystem
.\BankSystem.exe
```

> **注意**：必须在 `BankSystem\` 目录下运行，程序会在当前目录下自动创建 `data\` 文件夹存放数据，请确保有写权限。

---

## 默认账号

| 角色 | 账号 | 密码 |
|------|------|------|
| 管理员 | `admin` | `admin123` |

首次运行时系统自动创建管理员账号，并预置 6 个北京网点示例数据。

---

## 数据文件说明

程序运行后 `data\` 目录下会生成以下二进制文件：

| 文件 | 内容 |
|------|------|
| `employees.dat` | 职员信息 |
| `customers.dat` | 客户信息 |
| `cards.dat` | 银行卡信息 |
| `transactions.dat` | 交易记录 |
| `queue_stats.dat` | 排队每日统计 |
| `branches.dat` | 网点信息 |
| `branch_graph.dat` | 网点路径图 |

---

## 角色权限说明

| 功能 | 管理员 | 职员 | 客户 |
|------|:------:|:----:|:----:|
| 职员管理 | ✓ | — | — |
| 客户账户管理 | ✓ | ✓ | 查看自己 |
| 银行卡管理 | ✓ | ✓ | 查看自己 |
| 存贷款业务 | ✓ | ✓ | ✓ |
| 业务查询 | ✓ | ✓ | 查看自己 |
| 排队管理 | ✓ | ✓ | 取号 |
| 网点导航 | ✓ | ✓ | ✓ |
| 智能管理 | ✓ | ✓ | — |
