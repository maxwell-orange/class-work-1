#pragma once
#include "types.h"
#include "datastructs.h"

/*=============================================================
 *  文件 I/O 模块
 *  使用二进制方式存储固定大小结构体
 *=============================================================*/

// 泛型：将链表保存到文件
template<typename T>
bool saveList(const char* filepath, const LinkedList<T>& list) {
    FILE* f = fopen(filepath, "wb");
    if (!f) return false;
    int cnt = list.size();
    fwrite(&cnt, sizeof(int), 1, f);
    list.forEach([&](const T& item) {
        fwrite(&item, sizeof(T), 1, f);
    });
    fclose(f);
    return true;
}

// 泛型：从文件加载到链表
template<typename T>
bool loadList(const char* filepath, LinkedList<T>& list) {
    FILE* f = fopen(filepath, "rb");
    if (!f) return false;
    int cnt = 0;
    if (fread(&cnt, sizeof(int), 1, f) != 1) { fclose(f); return false; }
    for (int i = 0; i < cnt; ++i) {
        T item;
        if (fread(&item, sizeof(T), 1, f) == 1)
            list.push_back(item);
    }
    fclose(f);
    return true;
}

// 图的持久化
bool saveGraph(const char* filepath, const Graph& g);
bool loadGraph(const char* filepath, Graph& g);

// 确保 data 目录存在
void ensureDataDir();
