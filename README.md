# AED Proyecto 2 — R-Tree: Buscador espacial

## Descripción

Buscador espacial sobre puntos de interés geográficos (ciudades de GeoNames y
heladerías Grido extraídas de OpenStreetMap), indexados con un **R-Tree clásico
con split cuadrático** implementado desde cero.

La aplicación muestra un mapa interactivo que permite:

- **Consultas por rango rectangular**: arrastrando una región sobre el mapa.
- **Consultas KNN** (k vecinos más cercanos): con click derecho, k ajustable en vivo.
- **Inserción y eliminación** de puntos en tiempo real (con condensación del árbol).
- **Visualización de los MBRs** de los nodos visitados en cada consulta, como
  evidencia de la poda del árbol.
- **Comparación contra búsqueda lineal**: cada consulta reporta tiempo y
  cantidad de nodos visitados por el R-Tree vs elementos revisados linealmente.

Estructuras de datos (`Vector`, `MinHeap`, etc.) implementadas a mano en `include/`.

### Estructura del proyecto

```
include/rtree.hpp    R-Tree: insert, remove, rangeQuery, knn, split cuadrático
include/             estructuras auxiliares (vector, minheap, stack, queue, dolist)
mapa/                visualización SDL3 (MapView, DataLoader, Window, cámara)
main.cpp             aplicación gráfica
bench.cpp            benchmark por consola (experimentos del reporte, sin SDL)
tests/               tests unitarios con Catch2
assets/              datasets: cities15000.txt (GeoNames) y grido.json (OSM)
```

## Integrantes

- Castillo Prado, Leonardo Humberto
- Mattos Gutierrez, Angel Daniel
- Dasso Arana, Alvaro Jose

## Compilación

Requiere CMake ≥ 3.10, un compilador con C++20, y SDL3 + SDL3_ttf + SDL3_image
(solo para la aplicación gráfica; los tests y el benchmark no usan SDL).

### macOS

```bash
brew install cmake sdl3 sdl3_ttf sdl3_image
cmake -B build
cmake --build build
```

### Linux / Nix

```bash
nix develop          # entorno con todas las dependencias
cmake -B build
cmake --build build
```

Se generan tres ejecutables: `proyecto2` (app gráfica), `bench` (experimentos)
y `tests/tests` (tests unitarios).

## Ejecución

### Aplicación gráfica

```bash
./build/proyecto2
```

| Acción | Control |
|---|---|
| Consulta por rango | Arrastrar con click izquierdo |
| Consulta KNN | Click derecho |
| Cambiar k del KNN | Flechas ↑ / ↓ |
| Insertar punto | Shift + click izquierdo |
| Eliminar punto más cercano | Shift + click derecho |
| Mostrar/ocultar MBRs visitados | Tecla M |
| Zoom | Rueda del mouse |
| Pan | Arrastrar con click medio |

El panel inferior muestra, para cada consulta, el tiempo y número de resultados
del R-Tree y de la búsqueda lineal, junto con los nodos visitados por el árbol
vs los elementos revisados linealmente.

### Benchmark (experimentos)

```bash
./build/bench assets/cities15000.txt
```

Corre los tres experimentos del reporte y exporta `resultados_benchmark.csv`:

1. R-Tree vs lineal con datasets de 1,000 / 5,000 / 15,000 objetos
   (tiempo de construcción, promedio de range query y de KNN).
2. Efecto del tamaño de la ventana de consulta (1° a 64°).
3. Efecto del valor de k en KNN (1 a 100).

### Tests

```bash
./build/tests/tests            # toda la suite
./build/tests/tests "[rtree]"  # solo los del R-Tree
```

Los tests del R-Tree validan las consultas contra búsqueda lineal con datos
aleatorios, la consistencia de MBRs y punteros tras eliminaciones, y la
ocupación mínima/máxima de nodos después de los splits.
