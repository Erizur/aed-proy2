// Benchmark R-Tree vs busqueda lineal (sin SDL, solo consola)
//
// Uso:   ./bench [ruta a cities15000.txt] [salida.csv]
// Corre los experimentos pedidos en el enunciado:
//   - 3 tamanos de dataset (1000, 5000, 15000)
//   - tiempo de construccion del R-Tree
//   - tiempo promedio de range query y de KNN (R-Tree vs lineal)
//   - nodos visitados por el R-Tree vs elementos revisados por lineal
//   - efecto del tamano del rectangulo de consulta y del valor de k

#include "include/rtree.hpp"
#include "include/vector.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <random>
#include <sstream>
#include <string>

using Clock = std::chrono::high_resolution_clock;

static double elapsedMs(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}

// Loader de GeoNames igual al de mapa/DataLoader.hpp pero sin SDL
static Vector<SpatialObject> loadGeoNames(const std::string &path,
                                          int maxCount = -1) {
  Vector<SpatialObject> result;
  std::ifstream file(path);
  if (!file.is_open()) {
    std::printf("No se pudo abrir %s\n", path.c_str());
    return result;
  }

  std::string line;
  int count = 0;
  while (std::getline(file, line)) {
    if (maxCount >= 0 && count >= maxCount)
      break;
    if (line.empty())
      continue;

    Vector<std::string> cols;
    std::istringstream ss(line);
    std::string col;
    while (std::getline(ss, col, '\t'))
      cols.push_back(col);
    if (cols.size() < 9)
      continue;

    try {
      SpatialObject obj;
      obj.id = std::stoi(cols[0]);
      obj.name = cols[1];
      obj.category = cols[8];
      obj.y = std::stod(cols[4]); // lat
      obj.x = std::stod(cols[5]); // lon
      result.push_back(obj);
      count++;
    } catch (...) {
      continue;
    }
  }
  return result;
}

struct RangeResult {
  double rtreeMs, linearMs;
  double rtreeVisited, linearVisited;
  double results;
};

struct KnnResult {
  double rtreeMs, linearMs;
  double rtreeVisited, linearVisited;
};

// Promedia nQueries range queries con ventana de lado 'windowDeg',
// centradas en puntos aleatorios del dataset (zonas con datos reales)
static RangeResult benchRange(RTree &tree, const Vector<SpatialObject> &objs,
                              double windowDeg, int nQueries,
                              std::mt19937 &rng) {
  std::uniform_int_distribution<size_t> pick(0, objs.size() - 1);
  RangeResult acc{0, 0, 0, 0, 0};

  for (int q = 0; q < nQueries; q++) {
    const SpatialObject &c = objs[pick(rng)];
    double half = windowDeg / 2.0;
    double minX = c.x - half, maxX = c.x + half;
    double minY = c.y - half, maxY = c.y + half;

    auto t0 = Clock::now();
    Vector<Entry> r = tree.rangeQuery(minX, minY, maxX, maxY);
    auto t1 = Clock::now();
    acc.rtreeMs += elapsedMs(t0, t1);
    acc.rtreeVisited += tree.lastVisited;
    acc.results += (double)r.size();

    // volatile evita que el compilador elimine el bucle lineal
    static volatile int sink = 0;
    int found = 0;
    auto t2 = Clock::now();
    for (size_t i = 0; i < objs.size(); i++) {
      const SpatialObject &o = objs[i];
      if (o.x >= minX && o.x <= maxX && o.y >= minY && o.y <= maxY)
        found++;
    }
    auto t3 = Clock::now();
    sink = sink + found;
    acc.linearMs += elapsedMs(t2, t3);
    acc.linearVisited += (double)objs.size();
  }

  acc.rtreeMs /= nQueries;
  acc.linearMs /= nQueries;
  acc.rtreeVisited /= nQueries;
  acc.linearVisited /= nQueries;
  acc.results /= nQueries;
  return acc;
}

// Promedia nQueries consultas KNN con el k dado
static KnnResult benchKnn(RTree &tree, const Vector<SpatialObject> &objs,
                          int k, int nQueries, std::mt19937 &rng) {
  std::uniform_int_distribution<size_t> pick(0, objs.size() - 1);
  KnnResult acc{0, 0, 0, 0};

  for (int q = 0; q < nQueries; q++) {
    const SpatialObject &c = objs[pick(rng)];

    auto t0 = Clock::now();
    Vector<Entry> r = tree.knn(c.x, c.y, k);
    auto t1 = Clock::now();
    acc.rtreeMs += elapsedMs(t0, t1);
    acc.rtreeVisited += tree.lastVisited;
    (void)r;

    // lineal: distancia a todos + seleccion de los k menores
    auto t2 = Clock::now();
    Vector<std::pair<double, int>> all;
    for (size_t i = 0; i < objs.size(); i++) {
      double dx = objs[i].x - c.x, dy = objs[i].y - c.y;
      all.push_back({dx * dx + dy * dy, (int)i});
    }
    // seleccion parcial de los k menores
    for (int i = 0; i < k && i < (int)all.size(); i++) {
      int best = i;
      for (int j = i + 1; j < (int)all.size(); j++)
        if (all[j].first < all[best].first)
          best = j;
      std::swap(all[i], all[best]);
    }
    auto t3 = Clock::now();
    acc.linearMs += elapsedMs(t2, t3);
    acc.linearVisited += (double)objs.size();
  }

  acc.rtreeMs /= nQueries;
  acc.linearMs /= nQueries;
  acc.rtreeVisited /= nQueries;
  acc.linearVisited /= nQueries;
  return acc;
}

int main(int argc, char **argv) {
  std::string path = argc > 1 ? argv[1] : "assets/cities15000.txt";
  std::string csvPath = argc > 2 ? argv[2] : "resultados_benchmark.csv";

  Vector<SpatialObject> allObjs = loadGeoNames(path);
  if (allObjs.empty()) {
    std::printf("Dataset vacio, abortando.\n");
    return 1;
  }
  std::printf("Dataset total: %d objetos\n\n", (int)allObjs.size());

  std::ofstream csv(csvPath);
  csv << "experimento,n,parametro,rtree_ms,lineal_ms,rtree_nodos_visitados,"
         "lineal_elementos,resultados_prom,build_ms,nodos_arbol\n";

  const int N_QUERIES = 100;
  const int sizes[] = {1000, 5000, 15000};
  std::mt19937 rng(42); // semilla fija para reproducibilidad

  // ---------- Experimento 1: escalar el dataset ----------
  std::printf("=== Experimento 1: R-Tree vs lineal por tamano de dataset ===\n");
  std::printf("%8s %10s %12s %12s %12s %14s %14s\n", "n", "build(ms)",
              "range RT", "range LIN", "knn RT", "knn LIN", "nodos visit.");

  for (int s = 0; s < 3; s++) {
    int n = sizes[s];
    if (n > (int)allObjs.size())
      n = (int)allObjs.size();

    Vector<SpatialObject> objs;
    for (int i = 0; i < n; i++)
      objs.push_back(allObjs[i]);

    RTree tree(8);
    auto b0 = Clock::now();
    for (size_t i = 0; i < objs.size(); i++)
      tree.insert(objs[i].id, objs[i].name, objs[i].x, objs[i].y,
                  objs[i].category);
    auto b1 = Clock::now();
    double buildMs = elapsedMs(b0, b1);
    int totalNodes = tree.countNodes();

    RangeResult rr = benchRange(tree, objs, 4.0, N_QUERIES, rng);
    KnnResult kr = benchKnn(tree, objs, 10, N_QUERIES, rng);

    std::printf("%8d %10.2f %10.4fms %10.4fms %10.4fms %12.4fms %8.1f/%d\n", n,
                buildMs, rr.rtreeMs, rr.linearMs, kr.rtreeMs, kr.linearMs,
                rr.rtreeVisited, totalNodes);

    csv << "escala_range," << n << ",ventana=4deg," << rr.rtreeMs << ","
        << rr.linearMs << "," << rr.rtreeVisited << "," << rr.linearVisited
        << "," << rr.results << "," << buildMs << "," << totalNodes << "\n";
    csv << "escala_knn," << n << ",k=10," << kr.rtreeMs << "," << kr.linearMs
        << "," << kr.rtreeVisited << "," << kr.linearVisited << ",," << buildMs
        << "," << totalNodes << "\n";
  }

  // ---------- Experimentos 2 y 3 sobre el dataset completo ----------
  int n = (int)allObjs.size();
  RTree tree(8);
  for (size_t i = 0; i < allObjs.size(); i++)
    tree.insert(allObjs[i].id, allObjs[i].name, allObjs[i].x, allObjs[i].y,
                allObjs[i].category);

  std::printf("\n=== Experimento 2: efecto del tamano de la ventana (n=%d) ===\n", n);
  std::printf("%12s %12s %12s %14s %12s\n", "ventana(deg)", "rtree(ms)",
              "lineal(ms)", "nodos visit.", "result.prom");
  const double windows[] = {1.0, 4.0, 16.0, 64.0};
  for (double w : windows) {
    RangeResult rr = benchRange(tree, allObjs, w, N_QUERIES, rng);
    std::printf("%12.1f %12.4f %12.4f %14.1f %12.1f\n", w, rr.rtreeMs,
                rr.linearMs, rr.rtreeVisited, rr.results);
    csv << "efecto_ventana," << n << ",ventana=" << w << "deg," << rr.rtreeMs
        << "," << rr.linearMs << "," << rr.rtreeVisited << ","
        << rr.linearVisited << "," << rr.results << ",,\n";
  }

  std::printf("\n=== Experimento 3: efecto de k en KNN (n=%d) ===\n", n);
  std::printf("%6s %12s %12s %14s\n", "k", "rtree(ms)", "lineal(ms)",
              "nodos visit.");
  const int ks[] = {1, 5, 20, 100};
  for (int k : ks) {
    KnnResult kr = benchKnn(tree, allObjs, k, N_QUERIES, rng);
    std::printf("%6d %12.4f %12.4f %14.1f\n", k, kr.rtreeMs, kr.linearMs,
                kr.rtreeVisited);
    csv << "efecto_k," << n << ",k=" << k << "," << kr.rtreeMs << ","
        << kr.linearMs << "," << kr.rtreeVisited << "," << kr.linearVisited
        << ",,,\n";
  }

  std::printf("\nResultados guardados en %s\n", csvPath.c_str());
  return 0;
}
