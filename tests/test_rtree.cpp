#include "catch.hpp"
#include "rtree.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

// ---------- helpers ----------

// genera n puntos aleatorios reproducibles
static std::vector<SpatialObject> randomPoints(int n, unsigned seed = 42) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> dx(-180.0, 180.0);
  std::uniform_real_distribution<double> dy(-90.0, 90.0);

  std::vector<SpatialObject> pts;
  for (int i = 0; i < n; i++) {
    SpatialObject o;
    o.id = i;
    o.name = "p" + std::to_string(i);
    o.category = "test";
    o.x = dx(rng);
    o.y = dy(rng);
    pts.push_back(o);
  }
  return pts;
}

static void insertAll(RTree &t, const std::vector<SpatialObject> &pts) {
  for (const auto &o : pts)
    t.insert(o.id, o.name, o.x, o.y, o.category);
}

// ids esperados de un range query hecho a mano
static std::vector<int> linearRange(const std::vector<SpatialObject> &pts,
                                    double minX, double minY, double maxX,
                                    double maxY) {
  std::vector<int> ids;
  for (const auto &o : pts)
    if (o.x >= minX && o.x <= maxX && o.y >= minY && o.y <= maxY)
      ids.push_back(o.id);
  std::sort(ids.begin(), ids.end());
  return ids;
}

static std::vector<int> idsOf(const Vector<Entry> &entries) {
  std::vector<int> ids;
  for (size_t i = 0; i < entries.size(); i++)
    ids.push_back(entries[i].obj.id);
  std::sort(ids.begin(), ids.end());
  return ids;
}

// verifica que el MBR de cada nodo cubre a sus entradas (invariante del arbol)
static bool checkMBRs(RTNode *node) {
  if (node == nullptr || node->entries.empty())
    return true;
  MBR computed = node->entries[0].mbr;
  for (size_t i = 1; i < node->entries.size(); i++)
    computed = computed.expand(node->entries[i].mbr);

  if (computed.minX != node->mbr.minX || computed.minY != node->mbr.minY ||
      computed.maxX != node->mbr.maxX || computed.maxY != node->mbr.maxY)
    return false;

  if (!node->isLeaf)
    for (size_t i = 0; i < node->entries.size(); i++) {
      if (node->entries[i].child->parent != node)
        return false; // punteros al padre consistentes
      if (!checkMBRs(node->entries[i].child))
        return false;
    }
  return true;
}

// cuenta objetos en las hojas
static int countObjects(RTNode *node) {
  if (node == nullptr)
    return 0;
  if (node->isLeaf)
    return (int)node->entries.size();
  int total = 0;
  for (size_t i = 0; i < node->entries.size(); i++)
    total += countObjects(node->entries[i].child);
  return total;
}

// ---------- tests ----------

TEST_CASE("RTree: rangeQuery coincide con busqueda lineal", "[rtree]") {
  auto pts = randomPoints(500);
  RTree tree(4);
  insertAll(tree, pts);

  REQUIRE(checkMBRs(tree.root));
  REQUIRE(countObjects(tree.root) == 500);

  std::mt19937 rng(7);
  std::uniform_real_distribution<double> dx(-180.0, 160.0);
  std::uniform_real_distribution<double> dy(-90.0, 70.0);

  for (int q = 0; q < 20; q++) {
    double minX = dx(rng), minY = dy(rng);
    double maxX = minX + 20.0, maxY = minY + 20.0;

    auto expected = linearRange(pts, minX, minY, maxX, maxY);
    auto got = idsOf(tree.rangeQuery(minX, minY, maxX, maxY));
    REQUIRE(got == expected);
  }
}

TEST_CASE("RTree: knn coincide con busqueda lineal", "[rtree]") {
  auto pts = randomPoints(300, 99);
  RTree tree(4);
  insertAll(tree, pts);

  std::mt19937 rng(13);
  std::uniform_real_distribution<double> dx(-180.0, 180.0);
  std::uniform_real_distribution<double> dy(-90.0, 90.0);

  for (int q = 0; q < 10; q++) {
    double x = dx(rng), y = dy(rng);
    int k = 1 + q; // varios valores de k

    // knn lineal por distancia
    std::vector<std::pair<double, int>> all;
    for (const auto &o : pts) {
      double d = std::hypot(o.x - x, o.y - y);
      all.push_back({d, o.id});
    }
    std::sort(all.begin(), all.end());

    auto got = tree.knn(x, y, k);
    REQUIRE((int)got.size() == k);

    // comparamos distancias (puede haber empates con ids distintos)
    for (int i = 0; i < k; i++) {
      double d = std::hypot(got[i].obj.x - x, got[i].obj.y - y);
      REQUIRE(d == Approx(all[i].first));
    }
    // y en orden ascendente
    for (int i = 1; i < k; i++) {
      double dPrev = std::hypot(got[i - 1].obj.x - x, got[i - 1].obj.y - y);
      double dCur = std::hypot(got[i].obj.x - x, got[i].obj.y - y);
      REQUIRE(dPrev <= dCur);
    }
  }
}

TEST_CASE("RTree: knn con k mayor que el total", "[rtree]") {
  auto pts = randomPoints(5);
  RTree tree(4);
  insertAll(tree, pts);

  auto got = tree.knn(0, 0, 50);
  REQUIRE((int)got.size() == 5);
}

TEST_CASE("RTree: remove elimina y el arbol sigue consistente", "[rtree]") {
  auto pts = randomPoints(200, 5);
  RTree tree(4);
  insertAll(tree, pts);

  // eliminamos la mitad
  for (int i = 0; i < 100; i++)
    REQUIRE(tree.remove(pts[i].id, pts[i].x, pts[i].y));

  REQUIRE(checkMBRs(tree.root));
  REQUIRE(countObjects(tree.root) == 100);

  // los eliminados ya no aparecen; los demas si
  auto got = idsOf(tree.rangeQuery(-180, -90, 180, 90));
  REQUIRE((int)got.size() == 100);
  for (int id : got)
    REQUIRE(id >= 100);

  // eliminar algo inexistente devuelve false
  REQUIRE_FALSE(tree.remove(pts[0].id, pts[0].x, pts[0].y));
  REQUIRE_FALSE(tree.remove(99999, 0, 0));
}

TEST_CASE("RTree: remove de todos deja el arbol vacio y usable", "[rtree]") {
  auto pts = randomPoints(50, 11);
  RTree tree(4);
  insertAll(tree, pts);

  for (const auto &o : pts)
    REQUIRE(tree.remove(o.id, o.x, o.y));

  REQUIRE(tree.rangeQuery(-180, -90, 180, 90).empty());
  REQUIRE(tree.knn(0, 0, 3).empty());

  // se puede volver a insertar despues de vaciarlo
  tree.insert(1000, "nuevo", 10, 10, "test");
  auto got = tree.rangeQuery(9, 9, 11, 11);
  REQUIRE(got.size() == 1);
  REQUIRE(got[0].obj.id == 1000);
}

TEST_CASE("RTree: rangeQuery cuenta nodos visitados", "[rtree]") {
  auto pts = randomPoints(500);
  RTree tree(4);
  insertAll(tree, pts);

  // consulta chica: debe visitar menos nodos que el total del arbol
  tree.rangeQuery(0, 0, 5, 5);
  int visitedSmall = tree.lastVisited;
  REQUIRE(visitedSmall >= 1);
  REQUIRE(visitedSmall < tree.countNodes());
  REQUIRE((int)tree.visitedMBRs.size() == visitedSmall);

  // consulta que cubre todo: visita todos los nodos
  tree.rangeQuery(-180, -90, 180, 90);
  REQUIRE(tree.lastVisited == tree.countNodes());
}

TEST_CASE("RTree: split cuadratico respeta minimo de entradas", "[rtree]") {
  // insertar en orden que fuerza muchos splits
  RTree tree(4); // minSize = 2
  for (int i = 0; i < 100; i++)
    tree.insert(i, "p", (double)(i % 10), (double)(i / 10), "test");

  // recorremos el arbol validando ocupacion minima (excepto la raiz)
  Vector<RTNode *> stack;
  stack.push_back(tree.root);
  while (!stack.empty()) {
    RTNode *node = stack[stack.size() - 1];
    stack.pop_back();
    if (node != tree.root)
      REQUIRE((int)node->entries.size() >= tree.minSize);
    REQUIRE((int)node->entries.size() <= tree.maxSize);
    if (!node->isLeaf)
      for (size_t i = 0; i < node->entries.size(); i++)
        stack.push_back(node->entries[i].child);
  }
}
