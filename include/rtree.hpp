#ifndef RTREE_HPP
#define RTREE_HPP

#include "vector.hpp"
#include <string>

struct MBR {
  double minX, minY, maxX, maxY;
};

// forward declaration, solamente para tener en cuenta el puntero
// sino sale un error grande, bien grande :P
struct RTNode;

struct Entry {
  MBR mbr;
  RTNode *child;
  int id;
  std::string nameId;
};

struct RTNode {
  bool isLeaf;
  MBR mbr;
  Vector<Entry> entries;
  RTNode *root;
};

// TODO: terminar el rtree pq falta
struct RTree {
    RTNode* root;
    int maxSize;
    int minSize;

    RTree(int maxEntries = 4) : maxSize(maxEntries), minSize((maxEntries + 1) / 2) {
        root = new RTNode();
        root->isLeaf = true;
        root->root = nullptr;
    }

    void insert(int id, const std::string& name, double x, double y);
    Vector<Entry> rangeQuery(double minX, double minY, double maxX, double maxY);
    Vector<Entry> knn(double x, double y, int k);

private:
    RTNode* chooseLeaf(const MBR& mbr);
    std::pair<RTNode*, RTNode*> splitNode(RTNode* node);
    void adjustTree(RTNode* L, RTNode* LL);
    void updateMBR(RTNode* node);
};

#endif // RTREE_HPP