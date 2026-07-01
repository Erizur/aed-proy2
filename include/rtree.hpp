#ifndef RTREE_HPP
#define RTREE_HPP

#include "vector.hpp"
#include <algorithm>
#include <string>
#include <cmath>
#include <utility> // Necesario para std::pair, usado en splitNode

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))



struct SpatialObject {
  int id= -1; //Id unico del objeto
  std::string name; //Nombre visible del punto
  sdt::string category; // Categoria para filtrar o colorear
  double x=0.0; //Coordenada X del objeto
  double y=0.0; //Coordenada y del objeto
};

struct MBR {
  double minX=0.0, minY=0.0, maxX=0.0, maxY=0.0;

  double area() const { return (maxX - minX) * (maxY - minY); }// Calcula el area para comparar el crecimiento


  MBR expand(const MBR &other) const {
    return {
        std::min(minX, other.minX), //El nuevo borde izquierdo sera el menor minX
        std::min(minY, other.minY), 
        std::max(maxX, other.maxX),//El nuevo borde derecho sera el mayor maxX
        std::max(maxY, other.maxY)};
  }

  double enlarge(const MBR &other) const {
    return expand(other).area() - area(); //Mide cuanto creceria este MBR al incluir otro.
  }

  bool intersects(const MBR &other) const {
    return minX <= other.maxX && maxX >= other.minX && minY <= other.maxY &&
           maxY >= other.minY; //Verifica si dos rectangulos se cruzan
  }

  bool containsPoint(double x, double y) const{
    return x>=minX && x<=maxX && y>= minY && y<=maxY; // Verifica si un punto cae dentro
  }

  double distanceTo(double x, double y) const {
    double closestX = std::max(minX, std::min(x, maxX));//Proyecta x al borde mas cercano del MBR
    double closestY = std::max(minY, std::min(y, maxY));//Proyecta y al borde mas cercano del MBR
    double dx = x - closestX; //Distancia horizontal al MBR
    double dy = y - closestY;
    return std::sqrt(dx*dx + dy*dy);//Distancia euclidiana minima al MBR
  }
};

// forward declaration, solamente para tener en cuenta el puntero
// sino sale un error grande, bien grande :P
struct RTNode;

struct Entry {
  MBR mbr; // Rectangulo que cubre al hijo o al objeto, representa al hijo o al objeto
  RTNode *child= nullptr;//Puntero al hijo si la entrada esta en un nodo interno
  SpatialObject object;//Objeto real si la entrada esta en una hoja
  bool hasObject =false; //Indica si la entrada guarda un objeto real
};

struct RTNode {
  bool isLeaf = true; //Indica si el nodo es hoja
  MBR mbr;// MBR que cubre todas las entradas del nodo
  Vector<Entry> entries;// Entradas almacenadas en el nodo
  RTNode *parent = nullptr;// Padre del nodo para actualizar MBRs hacia arriba
};

// TODO: terminar el rtree pq falta
struct RTree {
  RTNode *root=nullptr;//Puntero a raiz del RTree
  int maxSize =4;//Maximo de entradas permitidas por nodo
  int minSize=2;//ceil(maxSize/2)

  RTree(int maxEntries = 4);//Constructor(inicializa la raiz)

  void insert(int id, const std::string &name, double x, double y,const std::string& category="default");//Inserta un objeto espacial

  Vector<Entry> rangeQuery(double minX, double minY, double maxX, double maxY);//Busca objetos dentro del rectangulo

  Vector<Entry> knn(double x, double y, int k);//K nearest Neighbors

private:
  RTNode *chooseLeaf(const MBR &mbr);// Elige la mejor hoja para insertar
  std::pair<RTNode *, RTNode *> splitNode(RTNode *node);// Divide un nodo con overflow
  void adjustTree(RTNode *L, RTNode *LL);//Actualiza MBRs y propaga el split
  void updateMBR(RTNode *node);//Recalcula el MBR de un nodo
};

MBR pointMBR(double x,double y);//Para insertar un punto, lo convertimos a MBR degenerado. Es una funcion auxiliar simple, no necesita acceder a root, maxSize, ni al estado interno del arbol. Por eso esta fuera del struct Tree
#endif // RTREE_HPP