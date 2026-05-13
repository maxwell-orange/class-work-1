#pragma once
#include <cstdlib>
#include <cstring>
#include <functional>
#include <stdexcept>

/*=============================================================
 *  1. 双向链表  LinkedList<T>
 *     - push_back / push_front
 *     - remove_if(pred)
 *     - find_if(pred) -> T* (nullptr if not found)
 *     - forEach(callback)
 *     - size() / clear()
 *=============================================================*/
template<typename T>
class LinkedList {
public:
    struct Node {
        T    data;
        Node* prev;
        Node* next;
        explicit Node(const T& d) : data(d), prev(nullptr), next(nullptr) {}
    };

    LinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}
    ~LinkedList() { clear(); }

    // 禁止拷贝
    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    void push_back(const T& val) {
        Node* n = new Node(val);
        if (!tail_) { head_ = tail_ = n; }
        else { tail_->next = n; n->prev = tail_; tail_ = n; }
        ++size_;
    }
    void push_front(const T& val) {
        Node* n = new Node(val);
        if (!head_) { head_ = tail_ = n; }
        else { n->next = head_; head_->prev = n; head_ = n; }
        ++size_;
    }

    // 删除所有满足谓词的节点，返回删除数量
    int remove_if(std::function<bool(const T&)> pred) {
        int cnt = 0;
        Node* cur = head_;
        while (cur) {
            Node* nxt = cur->next;
            if (pred(cur->data)) {
                unlink(cur);
                delete cur;
                ++cnt;
            }
            cur = nxt;
        }
        return cnt;
    }

    // 找到第一个满足谓词的元素，返回指针（可修改）
    T* find_if(std::function<bool(const T&)> pred) {
        for (Node* cur = head_; cur; cur = cur->next)
            if (pred(cur->data)) return &cur->data;
        return nullptr;
    }

    // 遍历（只读）
    void forEach(std::function<void(const T&)> cb) const {
        for (Node* cur = head_; cur; cur = cur->next) cb(cur->data);
    }
    // 遍历（可修改）
    void forEachMut(std::function<void(T&)> cb) {
        for (Node* cur = head_; cur; cur = cur->next) cb(cur->data);
    }

    // 将满足谓词的元素复制到 out 数组，返回个数
    int collect(std::function<bool(const T&)> pred, T* out, int maxOut) const {
        int cnt = 0;
        for (Node* cur = head_; cur && cnt < maxOut; cur = cur->next)
            if (pred(cur->data)) out[cnt++] = cur->data;
        return cnt;
    }

    int  size()  const { return size_; }
    bool empty() const { return size_ == 0; }

    void clear() {
        Node* cur = head_;
        while (cur) { Node* nxt = cur->next; delete cur; cur = nxt; }
        head_ = tail_ = nullptr;
        size_ = 0;
    }

    Node* begin() const { return head_; }

private:
    Node* head_;
    Node* tail_;
    int   size_;

    void unlink(Node* n) {
        if (n->prev) n->prev->next = n->next; else head_ = n->next;
        if (n->next) n->next->prev = n->prev; else tail_ = n->prev;
        --size_;
    }
};

/*=============================================================
 *  2. 栈  Stack<T>（基于链表）
 *=============================================================*/
template<typename T>
class Stack {
    struct Node { T data; Node* next; };
    Node* top_;
    int   size_;
public:
    Stack() : top_(nullptr), size_(0) {}
    ~Stack() { clear(); }

    void push(const T& val) {
        Node* n = new Node{val, top_};
        top_ = n; ++size_;
    }
    T pop() {
        if (!top_) throw std::underflow_error("Stack is empty");
        T v = top_->data;
        Node* old = top_; top_ = top_->next;
        delete old; --size_;
        return v;
    }
    const T& peek() const {
        if (!top_) throw std::underflow_error("Stack is empty");
        return top_->data;
    }
    bool empty() const { return size_ == 0; }
    int  size()  const { return size_; }
    void clear() {
        while (top_) { Node* n = top_->next; delete top_; top_ = n; }
        size_ = 0;
    }
};

/*=============================================================
 *  3. 队列  Queue<T>（链式队列）
 *=============================================================*/
template<typename T>
class Queue {
    struct Node { T data; Node* next; };
    Node* front_;
    Node* rear_;
    int   size_;
public:
    Queue() : front_(nullptr), rear_(nullptr), size_(0) {}
    ~Queue() { clear(); }

    void enqueue(const T& val) {
        Node* n = new Node{val, nullptr};
        if (!rear_) { front_ = rear_ = n; }
        else { rear_->next = n; rear_ = n; }
        ++size_;
    }
    T dequeue() {
        if (!front_) throw std::underflow_error("Queue is empty");
        T v = front_->data;
        Node* old = front_; front_ = front_->next;
        if (!front_) rear_ = nullptr;
        delete old; --size_;
        return v;
    }
    const T& front() const {
        if (!front_) throw std::underflow_error("Queue is empty");
        return front_->data;
    }
    bool empty() const { return size_ == 0; }
    int  size()  const { return size_; }
    void clear() {
        while (front_) { Node* n = front_->next; delete front_; front_ = n; }
        rear_ = nullptr; size_ = 0;
    }
    // 遍历
    void forEach(std::function<void(const T&, int)> cb) const {
        int i = 0;
        for (Node* cur = front_; cur; cur = cur->next) cb(cur->data, i++);
    }
};

/*=============================================================
 *  4. 优先队列  PriorityQueue<T>（最大堆，T 需实现 operator<）
 *     VIP 排在前面：T::priority() 大的先出队
 *=============================================================*/
template<typename T>
class PriorityQueue {
    T*  heap_;
    int size_;
    int cap_;

    void grow() {
        cap_ = cap_ * 2 + 2;
        T* nb = new T[cap_];
        for (int i = 0; i < size_; ++i) nb[i] = heap_[i];
        delete[] heap_; heap_ = nb;
    }
    void siftUp(int i) {
        while (i > 0) {
            int p = (i-1)/2;
            if (heap_[p] < heap_[i]) { std::swap(heap_[p],heap_[i]); i=p; }
            else break;
        }
    }
    void siftDown(int i) {
        while (true) {
            int l=2*i+1, r=2*i+2, m=i;
            if (l<size_ && heap_[m]<heap_[l]) m=l;
            if (r<size_ && heap_[m]<heap_[r]) m=r;
            if (m==i) break;
            std::swap(heap_[m],heap_[i]); i=m;
        }
    }
public:
    PriorityQueue() : heap_(nullptr), size_(0), cap_(0) {}
    ~PriorityQueue() { delete[] heap_; }

    void push(const T& val) {
        if (size_ >= cap_) grow();
        heap_[size_++] = val;
        siftUp(size_-1);
    }
    T pop() {
        if (!size_) throw std::underflow_error("PQ empty");
        T v = heap_[0];
        heap_[0] = heap_[--size_];
        if (size_) siftDown(0);
        return v;
    }
    const T& top() const { return heap_[0]; }
    bool empty() const { return size_==0; }
    int  size()  const { return size_; }
    void clear() { size_ = 0; }
    void forEach(std::function<void(const T&, int)> cb) const {
        for (int i=0;i<size_;++i) cb(heap_[i],i);
    }
};

/*=============================================================
 *  5. 图（邻接矩阵）Graph  —— 用于银行网点路由
 *     最大节点数 MAX_BRANCHES
 *     Dijkstra 最短路径
 *=============================================================*/
static const int GRAPH_INF = 0x3fffffff;

class Graph {
public:
    static const int MAXV = 50;

    struct Vertex {
        char id[24];
        char name[52];
        bool valid;
        Vertex() { memset(this,0,sizeof(*this)); }
    };

    Vertex verts[MAXV];
    double dist[MAXV][MAXV]; // 权重（km），0=自身，INF=无边
    int    vCnt;

    Graph() : vCnt(0) {
        for (int i=0;i<MAXV;++i)
            for (int j=0;j<MAXV;++j)
                dist[i][j] = (i==j) ? 0.0 : (double)GRAPH_INF;
    }

    // 添加节点，返回下标（-1=满）
    int addVertex(const char* id, const char* name) {
        if (vCnt >= MAXV) return -1;
        int idx = vCnt++;
        strncpy(verts[idx].id,   id,   23);
        strncpy(verts[idx].name, name, 51);
        verts[idx].valid = true;
        return idx;
    }

    // 查找节点下标（-1=不存在）
    int indexOf(const char* id) const {
        for (int i=0;i<vCnt;++i)
            if (verts[i].valid && strcmp(verts[i].id,id)==0) return i;
        return -1;
    }

    void addEdge(const char* from, const char* to, double w) {
        int a=indexOf(from), b=indexOf(to);
        if (a<0||b<0) return;
        dist[a][b] = dist[b][a] = w; // 无向图
    }
    void removeEdge(const char* from, const char* to) {
        int a=indexOf(from), b=indexOf(to);
        if (a<0||b<0) return;
        dist[a][b] = dist[b][a] = GRAPH_INF;
    }
    void removeVertex(const char* id) {
        int i = indexOf(id);
        if (i<0) return;
        verts[i].valid = false;
        for (int j=0;j<MAXV;++j) dist[i][j]=dist[j][i]=(double)GRAPH_INF;
    }

    // Dijkstra，返回 path 字符串（节点名依次），返回 false 表示不可达
    bool shortestPath(const char* from, const char* to,
                      char* pathBuf, int bufLen, double& totalDist) const {
        int s = indexOf(from), t = indexOf(to);
        if (s<0||t<0) return false;

        double d[MAXV];
        int    prev[MAXV];
        bool   visited[MAXV];
        for (int i=0;i<MAXV;++i){ d[i]=(double)GRAPH_INF; prev[i]=-1; visited[i]=false; }
        d[s]=0.0;

        for (int iter=0;iter<vCnt;++iter) {
            // 找未访问最小距离节点
            int u=-1;
            for (int i=0;i<vCnt;++i)
                if (verts[i].valid && !visited[i] && (u<0 || d[i]<d[u])) u=i;
            if (u<0 || d[u]>=(double)GRAPH_INF) break;
            visited[u]=true;
            for (int v=0;v<vCnt;++v) {
                if (!verts[v].valid||visited[v]) continue;
                double nd = d[u]+dist[u][v];
                if (nd < d[v]) { d[v]=nd; prev[v]=u; }
            }
        }

        if (d[t]>=(double)GRAPH_INF) return false;
        totalDist = d[t];

        // 回溯路径
        int path[MAXV], plen=0;
        for (int cur=t; cur>=0; cur=prev[cur]) path[plen++]=cur;
        // 反转
        char tmp[2048]="";
        for (int i=plen-1;i>=0;--i) {
            strncat(tmp, verts[path[i]].name, 51);
            if (i>0) strncat(tmp," -> ",5);
        }
        strncpy(pathBuf, tmp, bufLen-1);
        return true;
    }

    // 返回所有可达节点名，BFS
    int reachable(const char* from, char names[][52], int maxOut) const {
        int s = indexOf(from);
        if (s<0) return 0;
        bool visited[MAXV]={};
        // BFS
        int q[MAXV], head=0, tail=0, cnt=0;
        visited[s]=true; q[tail++]=s;
        while (head<tail) {
            int u=q[head++];
            if (u!=s && cnt<maxOut) strncpy(names[cnt++], verts[u].name, 51);
            for (int v=0;v<vCnt;++v) {
                if (verts[v].valid && !visited[v] && dist[u][v]<(double)GRAPH_INF) {
                    visited[v]=true; q[tail++]=v;
                }
            }
        }
        return cnt;
    }
};

/*=============================================================
 *  6. 广义表（GList）—— 用于客户账户结构展示
 *     原子节点存字符串，子表节点包含子表
 *=============================================================*/
struct GListNode {
    bool    isAtom;
    char    atom[64];
    GListNode* sublist;   // 子表头
    GListNode* next;      // 同层下一节点

    GListNode() : isAtom(true), sublist(nullptr), next(nullptr) {
        memset(atom, 0, sizeof(atom));
    }
};

class GList {
public:
    GListNode* head;
    GList() : head(nullptr) {}
    ~GList() { freeNode(head); }

    // 追加原子
    GListNode* appendAtom(const char* val, GListNode** tail = nullptr) {
        GListNode* n = new GListNode();
        n->isAtom = true;
        strncpy(n->atom, val, 63);
        return insertNode(n, tail);
    }
    // 追加子表，返回子表根节点
    GListNode* appendSublist(GListNode** tail = nullptr) {
        GListNode* n = new GListNode();
        n->isAtom = false;
        return insertNode(n, tail);
    }

    // 打印广义表
    void print(GListNode* node = nullptr, int depth = 0) const;

private:
    GListNode* insertNode(GListNode* n, GListNode** tail) {
        if (!head) { head = n; if(tail) *tail=n; return n; }
        GListNode* cur = head;
        while (cur->next) cur = cur->next;
        cur->next = n;
        if (tail) *tail = n;
        return n;
    }
    void freeNode(GListNode* n) {
        while (n) {
            if (!n->isAtom) freeNode(n->sublist);
            GListNode* nxt = n->next;
            delete n; n = nxt;
        }
    }
};
