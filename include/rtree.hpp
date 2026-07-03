#ifndef RTREE_HPP
#define RTREE_HPP

#include "minheap.hpp"
#include "vector.hpp"

#include <cmath>
#include <limits>  // std::numeric_limits, usado en chooseLeaf y splitNode
#include <string>
#include <utility> // std::pair, usado en splitNode

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

struct MBR {
  double minX = 0, minY = 0, maxX = 0, maxY = 0;

  double area() const { return (maxX - minX) * (maxY - minY); }

  double enlarge(const MBR &other) const {
    MBR expanded = expand(other);
    return expanded.area() - area();
  }

  MBR expand(const MBR &other) const {
    return {MIN(minX, other.minX), MIN(minY, other.minY), MAX(maxX, other.maxX),
            MAX(maxY, other.maxY)};
  }

  bool intersects(const MBR &other) const {
    return minX <= other.maxX && maxX >= other.minX && minY <= other.maxY &&
           maxY >= other.minY;
  }

  bool containsPoint(double x, double y) const {
    return x >= minX && x <= maxX && y >= minY &&
           y <= maxY; // Verifica si un punto cae dentro
  }

  double distanceTo(double x, double y) const {
    double cx = MAX(minX, MIN(x, maxX));
    double cy = MAX(minY, MIN(y, maxY));
    double dx = x - cx;                  // Distancia horizontal al MBR
    double dy = y - cy;                  // Distancia vertical al MBR
    return std::sqrt(dx * dx + dy * dy); // Distancia euclidiana minima al MBR
  }
};

// el static lo saque de mi jueguito y para evitar el error que prohibe las
// declaraciones fuera de clases en un headre
static MBR pointMBR(double x, double y) {
  return {x, y, x, y};
} // Para insertar un punto, lo convertimos a MBR degenerado. Es una funcion
  // auxiliar simple, no necesita acceder a root, maxSize, ni al estado interno
  // del arbol. Por eso esta fuera del struct Tree

struct SpatialObject {
  int id = -1;          // Id unico del objeto
  std::string name;     // Nombre visible del punto
  std::string category; // Categoria para filtrar o colorear
  double x = 0;         // Coordenada X del objeto
  double y = 0;         // Coordenada y del objeto
};

// forward declaration, solamente para tener en cuenta el puntero
// sino sale un error grande, bien grande :P
struct RTNode;

struct Entry {
  MBR mbr;
  RTNode *child =
      nullptr;       // Puntero al hijo si la entrada esta en un nodo interno
  SpatialObject obj; // Objeto real si la entrada esta en una hoja
  bool hasObject = false; // Indica si la entrada guarda un objeto real
};

struct RTNode {
  bool isLeaf = true; // Indica si el nodo es hoja
  MBR mbr;            // MBR que cubre todas las entradas del nodo
  Vector<Entry> entries;
  RTNode *parent = nullptr; // Padre del nodo para actualizar MBRs hacia arriba
};

struct RTree {
  RTNode *root = nullptr;
  int maxSize; // Maximo de entradas permitidas por nodo
  int minSize;

  // estadisticas de la ultima consulta (rangeQuery o knn)
  int lastVisited = 0;     // nodos del arbol visitados en la ultima consulta
  Vector<MBR> visitedMBRs; // MBRs de los nodos visitados, para dibujarlos

  RTree(int maxEntries = 4) // Constructor(inicializa la raiz)
      : maxSize(maxEntries), minSize((maxEntries + 1) / 2) {
    root = new RTNode();
    root->isLeaf = true;
    root->parent = nullptr;
  }

  ~RTree() { deleteNode(root); }

  void deleteNode(RTNode *node) {
    if (node == nullptr)
      return;
    if (!node->isLeaf) {
      for (size_t i = 0; i < node->entries.size(); i++)
        deleteNode(node->entries[i].child);
    }
    delete node;
  }

  void insert(int id, const std::string &name, double x, double y,
              const std::string &category) {
    MBR mbr = pointMBR(x, y);

    // construimos la entrada a insertar
    Entry e;
    e.mbr = mbr;
    e.child = nullptr;
    e.hasObject = true;
    e.obj = {id, name, category, x, y};

    // elegimos la hoja
    RTNode *leaf = chooseLeaf(mbr);
    leaf->entries.push_back(e);

    RTNode *newNode = nullptr;

    // si hay overflow, hacemos split
    if ((int)leaf->entries.size() > maxSize) {
      auto [splitA, splitB] = splitNode(leaf);

      // splitA reemplaza leaf en el padre
      RTNode *parent = leaf->parent;
      if (parent != nullptr) {
        for (size_t i = 0; i < parent->entries.size(); i++) {
          if (parent->entries[i].child == leaf) {
            parent->entries[i].child = splitA;
            parent->entries[i].mbr = splitA->mbr;
            splitA->parent = parent;
            break;
          }
        }
      } else {
        // leaf era la raiz
        root = splitA;
        splitA->parent = nullptr;
      }

      delete leaf;
      leaf = splitA;
      newNode = splitB;
    }

    adjustTree(leaf, newNode);
  } // Inserta un objeto espacial
  Vector<Entry> rangeQuery(double minX, double minY, double maxX, double maxY) {
    Vector<Entry> result;                 // Guardara las entradas encontradas
    MBR query = {minX, minY, maxX, maxY}; // Crea ek rectangulo de consulta

    lastVisited = 0; // Reinicia las estadisticas de la consulta
    visitedMBRs = Vector<MBR>();

    if (root == nullptr || root->entries.empty())
      return result;        // Si el arbol esta vacio
    Vector<RTNode *> stack; // Usamos un vector como pila de nodos por visitar
    stack.push_back(root);  // Empezamos desde la raiz
    while (!stack.empty()) {
      RTNode *node = stack[stack.size() - 1]; // Tomamos el ultimo nodo agregado
      stack.pop_back();

      lastVisited++; // Contamos cada nodo que realmente se revisa
      visitedMBRs.push_back(node->mbr);

      for (size_t i = 0; i < node->entries.size(); ++i) {
        Entry &entry =
            node->entries[i]; // Hacemos referencia a la entrada actual

        if (!entry.mbr.intersects(query))
          continue;         // Si no intersecta la consulta, se poda
        if (node->isLeaf) { // Si estamos en una hoja, la entrada es un objeto
          if (query.containsPoint(
                  entry.obj.x,
                  entry.obj.y)) {    // Verifica que el punto este dentro
            result.push_back(entry); // Agregamos el objeto al resultado
          }

        } else {
          stack.push_back(entry.child); // Agrega el hijo para revisarlo luego
        }
      }
    }
    return result;
  } // Busca objetos dentro del rectangulo

  // mini using solamente porque esa cosa se escribe bien feo
  using NodeEntry = std::pair<double, RTNode *>;
  using LeafEntry = std::pair<double, Entry>;

  Vector<Entry> knn(double x, double y, int k) {
    Vector<Entry> result;

    lastVisited = 0; // Reinicia las estadisticas de la consulta
    visitedMBRs = Vector<MBR>();

    if (root == nullptr)
      return result;

    struct ByDistNode {
      bool operator()(const NodeEntry &a, const NodeEntry &b) {
        return a.first < b.first; // min-heap: nodo mas cercano primero
      }
    };
    struct ByDistLeafMax {
      bool operator()(const LeafEntry &a, const LeafEntry &b) {
        return a.first > b.first; // max-heap: peor vecino en el top
      }
    };

    MinHeap<NodeEntry, ByDistNode> nodeHeap;
    MinHeap<LeafEntry, ByDistLeafMax> leafHeap; // guarda solo los k mejores

    nodeHeap.push({root->mbr.distanceTo(x, y), root});

    while (!nodeHeap.empty()) {
      auto [dist, node] = nodeHeap.pop();

      // poda: si el nodo esta mas lejos que el peor vecino actual, no sirve
      if ((int)leafHeap.size() >= k && dist > leafHeap.top().first)
        break;

      lastVisited++; // Solo cuentan los nodos que si se procesan
      visitedMBRs.push_back(node->mbr);

      if (node->isLeaf) {
        for (size_t i = 0; i < node->entries.size(); i++) {
          Entry &e = node->entries[i];
          double d = e.mbr.distanceTo(x, y);
          if ((int)leafHeap.size() < k) {
            leafHeap.push({d, e});
          } else if (d < leafHeap.top().first) {
            // encontramos uno mejor que el peor actual
            leafHeap.pop();
            leafHeap.push({d, e});
          }
        }
      } else {
        for (size_t i = 0; i < node->entries.size(); i++) {
          Entry &e = node->entries[i];
          double d = e.mbr.distanceTo(x, y);
          nodeHeap.push({d, e.child});
        }
      }
    }

    // extraemos del peor al mejor, luego invertimos
    while (!leafHeap.empty()) {
      result.push_back(leafHeap.pop().second);
    }

    // result esta en orden de mayor a menor distancia, invertimos
    for (size_t i = 0; i < result.size() / 2; i++) {
      Entry temp = result[i];
      result[i] = result[result.size() - 1 - i];
      result[result.size() - 1 - i] = temp;
    }

    return result;
  } // K nearest Neighbors

  bool remove(int id, double x, double y) {
    RTNode *leaf = findLeaf(root, id, x, y); // Busca la hoja que lo contiene
    if (leaf == nullptr)
      return false; // El objeto no existe en el arbol

    // quitamos la entrada de la hoja
    bool erased = false;
    for (size_t i = 0; i < leaf->entries.size(); i++) {
      const Entry &e = leaf->entries[i];
      if (e.hasObject && e.obj.id == id && e.obj.x == x && e.obj.y == y) {
        leaf->entries.erase(i);
        erased = true;
        break;
      }
    }
    if (!erased)
      return false;

    condenseTree(leaf); // Maneja underflow y actualiza MBRs hacia arriba

    // si la raiz quedo interna con un solo hijo, el hijo pasa a ser raiz
    if (!root->isLeaf && root->entries.size() == 1) {
      RTNode *oldRoot = root;
      root = oldRoot->entries[0].child;
      root->parent = nullptr;
      delete oldRoot; // delete simple: no recorre hijos como deleteNode
    }
    return true;
  } // Elimina un objeto espacial por id y coordenadas

  int countNodes() const { return countNodes(root); } // Total de nodos

private:
  int countNodes(RTNode *node) const {
    if (node == nullptr)
      return 0;
    int total = 1;
    if (!node->isLeaf)
      for (size_t i = 0; i < node->entries.size(); i++)
        total += countNodes(node->entries[i].child);
    return total;
  }

  RTNode *findLeaf(RTNode *node, int id, double x, double y) {
    if (node == nullptr)
      return nullptr;

    if (node->isLeaf) { // En una hoja buscamos el objeto exacto
      for (size_t i = 0; i < node->entries.size(); i++) {
        const Entry &e = node->entries[i];
        if (e.hasObject && e.obj.id == id && e.obj.x == x && e.obj.y == y)
          return node;
      }
      return nullptr;
    }

    // En nodos internos solo bajamos por hijos cuyo MBR contiene el punto
    for (size_t i = 0; i < node->entries.size(); i++) {
      if (node->entries[i].mbr.containsPoint(x, y)) {
        RTNode *found = findLeaf(node->entries[i].child, id, x, y);
        if (found != nullptr)
          return found;
      }
    }
    return nullptr;
  } // Busca la hoja que contiene un objeto

  void collectObjects(RTNode *node, Vector<SpatialObject> &out) {
    if (node == nullptr)
      return;
    if (node->isLeaf) {
      for (size_t i = 0; i < node->entries.size(); i++)
        out.push_back(node->entries[i].obj);
    } else {
      for (size_t i = 0; i < node->entries.size(); i++)
        collectObjects(node->entries[i].child, out);
    }
  } // Junta todos los objetos de un subarbol (para reinsertarlos)

  void condenseTree(RTNode *node) {
    Vector<SpatialObject> orphans; // Objetos que habra que reinsertar

    // Subimos desde la hoja hasta la raiz
    while (node != root) {
      RTNode *parent = node->parent;

      if ((int)node->entries.size() < minSize) {
        // underflow: sacamos el nodo del padre y guardamos sus objetos
        for (size_t i = 0; i < parent->entries.size(); i++) {
          if (parent->entries[i].child == node) {
            parent->entries.erase(i);
            break;
          }
        }
        collectObjects(node, orphans);
        deleteNode(node);
      } else {
        // sin underflow: solo actualizamos el MBR en el padre
        updateMBR(node);
        for (size_t i = 0; i < parent->entries.size(); i++) {
          if (parent->entries[i].child == node) {
            parent->entries[i].mbr = node->mbr;
            break;
          }
        }
      }
      node = parent;
    }
    updateMBR(root);

    // Reinsertamos los objetos huerfanos (estrategia clasica de Guttman)
    for (size_t i = 0; i < orphans.size(); i++) {
      const SpatialObject &o = orphans[i];
      insert(o.id, o.name, o.x, o.y, o.category);
    }
  } // Elimina nodos con underflow y reinserta sus objetos

  RTNode *chooseLeaf(const MBR &mbr) {
    RTNode *current = root; // La busqueda empieza desde la raiz

    while (current != nullptr &&
           !current->isLeaf) { // While que baja hasta encontrar una hoja
      int bestIndex = 0;
      double bestEnlarge =
          std::numeric_limits<double>::infinity(); // Mejor crecimiento
                                                   // encontrado
      double bestArea =
          std::numeric_limits<double>::infinity(); // Mejor area de desempate

      for (size_t i = 0; i < current->entries.size();
           ++i) { // Mira cada entrada del nodo actual
        double enlarge = current->entries[i].mbr.enlarge(
            mbr); // Calcula cuanto crecera ese hijo
        double area =
            current->entries[i].mbr.area(); // Calcula el area actual del hijo
        if (enlarge < bestEnlarge ||
            (enlarge == bestEnlarge &&
             area < bestArea)) { // Aplica el criterio de menor crecimiento
          bestIndex = i;         // Actualiza el menor hijo
          bestEnlarge = enlarge; // Guarda el menor crecimiento
          bestArea = area;       // Guarda el area usada para desempate
        }
      }
      current = current->entries[bestIndex].child; // Baja al hijo elegido.
    }
    return current; // Retornna la hoja donde insertara
  } // Elige la mejor hoja para insertar

  std::pair<RTNode *, RTNode *> splitNode(RTNode *node) {
    Vector<Entry> all = node->entries;

    // eleccion de semillas
    int seedA = 0, seedB = 1;
    double worstWaste = -std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < all.size(); i++) {
      for (size_t j = i + 1; j < all.size(); j++) {
        MBR combined = all[i].mbr.expand(all[j].mbr);
        double waste = combined.area() - all[i].mbr.area() - all[j].mbr.area();
        if (waste > worstWaste) {
          worstWaste = waste;
          seedA = i;
          seedB = j;
        }
      }
    }

    RTNode *groupA = new RTNode();
    RTNode *groupB = new RTNode();
    groupA->isLeaf = node->isLeaf;
    groupB->isLeaf = node->isLeaf;
    groupA->parent = node->parent;
    groupB->parent = node->parent;

    groupA->entries.push_back(all[seedA]);
    groupB->entries.push_back(all[seedB]);

    // opcion B: sin constructor con valor por defecto
    Vector<bool> assigned;
    for (size_t i = 0; i < all.size(); i++)
      assigned.push_back(false);
    assigned[seedA] = true;
    assigned[seedB] = true;

    int remaining = (int)all.size() - 2;

    // MBRs iniciales de cada grupo
    updateMBR(groupA);
    updateMBR(groupB);

    while (remaining > 0) {
      // forzar asignacion si un grupo necesita todas las restantes
      if ((int)groupA->entries.size() + remaining == minSize) {
        for (size_t i = 0; i < all.size(); i++) {
          if (!assigned[i]) {
            groupA->entries.push_back(all[i]);
            assigned[i] = true;
            remaining--;
          }
        }
        break;
      }
      if ((int)groupB->entries.size() + remaining == minSize) {
        for (size_t i = 0; i < all.size(); i++) {
          if (!assigned[i]) {
            groupB->entries.push_back(all[i]);
            assigned[i] = true;
            remaining--;
          }
        }
        break;
      }

      // pickNext: entrada con mayor diferencia de agrandamiento entre grupos
      int bestIdx = -1;
      double bestDiff = -std::numeric_limits<double>::infinity();
      for (size_t i = 0; i < all.size(); i++) {
        if (assigned[i])
          continue;
        double d1 = groupA->mbr.enlarge(all[i].mbr);
        double d2 = groupB->mbr.enlarge(all[i].mbr);
        double diff = std::abs(d1 - d2);
        if (diff > bestDiff) {
          bestDiff = diff;
          bestIdx = i;
        }
      }

      // asignar al grupo con menor agrandamiento, desempatar por area y tamaño
      double enlargeA = groupA->mbr.enlarge(all[bestIdx].mbr);
      double enlargeB = groupB->mbr.enlarge(all[bestIdx].mbr);

      if (enlargeA < enlargeB ||
          (enlargeA == enlargeB && groupA->mbr.area() < groupB->mbr.area()) ||
          (enlargeA == enlargeB && groupA->mbr.area() == groupB->mbr.area() &&
           groupA->entries.size() <= groupB->entries.size())) {
        groupA->entries.push_back(all[bestIdx]);
        updateMBR(groupA);
      } else {
        groupB->entries.push_back(all[bestIdx]);
        updateMBR(groupB);
      }

      assigned[bestIdx] = true;
      remaining--;
    }

    updateMBR(groupA);
    updateMBR(groupB);

    // actualizar parent de los hijos si es nodo interno
    // splitNode copia las entries por valor, pero los hijos siguen
    // apuntando al nodo original que sera eliminado por el caller
    if (!groupA->isLeaf) {
      for (size_t i = 0; i < groupA->entries.size(); i++)
        if (groupA->entries[i].child != nullptr)
          groupA->entries[i].child->parent = groupA;
    }
    if (!groupB->isLeaf) {
      for (size_t i = 0; i < groupB->entries.size(); i++)
        if (groupB->entries[i].child != nullptr)
          groupB->entries[i].child->parent = groupB;
    }

    return {groupA, groupB};
  } // Divide un nodo con overflow

  void adjustTree(RTNode *node, RTNode *newNode) {
    while (node != nullptr) {
      updateMBR(node);

      if (node == root) {
        if (newNode != nullptr) {
          RTNode *newRoot = new RTNode();
          newRoot->isLeaf = false;
          newRoot->parent = nullptr;

          Entry eA;
          eA.mbr = node->mbr;
          eA.child = node;
          Entry eB;
          eB.mbr = newNode->mbr;
          eB.child = newNode;

          newRoot->entries.push_back(eA);
          newRoot->entries.push_back(eB);

          node->parent = newRoot;
          newNode->parent = newRoot;
          updateMBR(newRoot);
          root = newRoot;
        }
        break;
      }

      RTNode *parent = node->parent;

      // actualizar la entrada del padre que apunta a node
      for (size_t i = 0; i < parent->entries.size(); i++) {
        if (parent->entries[i].child == node) {
          parent->entries[i].mbr = node->mbr;
          break;
        }
      }

      // propagar split si hubo
      RTNode *nextNew = nullptr;
      if (newNode != nullptr) {
        Entry e;
        e.mbr = newNode->mbr;
        e.child = newNode;
        newNode->parent = parent;
        parent->entries.push_back(e);

        if ((int)parent->entries.size() > maxSize) {
          auto [splitA, splitB] = splitNode(parent);
          RTNode *grandpa = parent->parent;
          splitA->parent = grandpa;
          splitB->parent = grandpa;

          if (grandpa != nullptr) {
            for (size_t i = 0; i < grandpa->entries.size(); i++) {
              if (grandpa->entries[i].child == parent) {
                grandpa->entries[i].child = splitA;
                grandpa->entries[i].mbr = splitA->mbr;
                break;
              }
            }
          } else {
            root = splitA;
          }

          delete parent;
          parent = splitA;
          nextNew = splitB;
        }
      }

      node = parent;
      newNode = nextNew;
    }
  } // Actualiza MBRs y propaga el split

  void updateMBR(RTNode *node) {
    if (node == nullptr || node->entries.empty())
      return; // No actualiza nodos nulos o vacios

    MBR current =
        node->entries[0].mbr; // Empieza con el MBR de la primera entrada

    for (size_t i = 1; i < node->entries.size();
         i++) { // Recorre las demas entradas del nodo
      current = current.expand(node->entries[i].mbr); // Expande el MBR
                                                      // acumulado
    }
    node->mbr = current; // Guarda el MBR total del nodo
  } // Recalcula el MBR de un nodo
};

#endif // RTREE_HPP