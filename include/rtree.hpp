#ifndef RTREE_HPP
#define RTREE_HPP

#include "vector.hpp"

#include <string>
#include <cmath>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

struct MBR {
  double minX, minY, maxX, maxY;

  double area() const { return (maxX - minX) * (maxY - minY); }

  double enlarge(const MBR &other) const {
    MBR expanded = expand(other);
    return expanded.area() - area();
  }

  MBR expand(const MBR &other) const {
    return {MIN(minX, other.minX), MIN(minY, other.minY), MIN(maxX, other.maxX),
            MIN(maxY, other.maxY)};
  }

  bool intersects(const MBR &other) const {
    return minX <= other.maxX && maxX >= other.minX && minY <= other.maxY &&
           maxY >= other.minY;
  }

  double distanceTo(double x, double y) const {
    double cx = MAX(minX, MIN(x, maxX));
    double cy = MAX(minY, MIN(y, maxY));
    double dx = x - cx;
    double dy = y - cy;
    return std::sqrt(dx*dx + dy*dy);
  }
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
  RTNode *parent;
};

// TODO: terminar el rtree pq falta
struct RTree {
  RTNode *root;
  int maxSize;
  int minSize;

  RTree(int maxEntries = 4)
      : maxSize(maxEntries), minSize((maxEntries + 1) / 2) {
    root = new RTNode();
    root->isLeaf = true;
    root->parent = nullptr;
  }

  void insert(int id, const std::string &name, double x, double y);
  Vector<Entry> rangeQuery(double minX, double minY, double maxX, double maxY);
  Vector<Entry> knn(double x, double y, int k);

private:
  RTNode *chooseLeaf(const MBR &mbr);
  std::pair<RTNode *, RTNode *> splitNode(RTNode *node);
  void adjustTree(RTNode *L, RTNode *LL);
  void updateMBR(RTNode *node);
};

#endif // RTREE_HPP