#include "fileio.h"
#include <cstdio>
#include <direct.h>   // _mkdir

bool saveGraph(const char* filepath, const Graph& g) {
    FILE* f = fopen(filepath, "wb");
    if (!f) return false;
    fwrite(&g.vCnt, sizeof(int), 1, f);
    fwrite(g.verts, sizeof(Graph::Vertex), Graph::MAXV, f);
    fwrite(g.dist,  sizeof(double), Graph::MAXV * Graph::MAXV, f);
    fclose(f);
    return true;
}

bool loadGraph(const char* filepath, Graph& g) {
    FILE* f = fopen(filepath, "rb");
    if (!f) return false;
    fread(&g.vCnt, sizeof(int), 1, f);
    fread(g.verts, sizeof(Graph::Vertex), Graph::MAXV, f);
    fread(g.dist,  sizeof(double), Graph::MAXV * Graph::MAXV, f);
    fclose(f);
    return true;
}

void ensureDataDir() {
    _mkdir("data");
}
