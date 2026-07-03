#pragma once

#include "DataLoader.hpp"
#include "MapCamera.hpp"
#include "Window.hpp"
#include "rtree.hpp"
#include "vector.hpp"

#include <chrono>
#include <cstdio> // snprintf para formatear tiempos
#include <string>

#include <SDL3/SDL.h>

// colores especificos del mapa
namespace MapColors {
    constexpr Color City       = {100, 149, 237, 180}; // azul clarito
    constexpr Color Heladeria  = {255, 105, 180, 220}; // rosa
    constexpr Color Custom     = {255, 255, 255, 230}; // punto insertado a mano
    constexpr Color ResultCity = {50,  220, 100, 255}; // verde resultado
    constexpr Color ResultHel  = {255, 220,  50, 255}; // amarillo resultado
    constexpr Color QueryRect  = {255, 200,  50,  60}; // rango drag
    constexpr Color QueryBorder= {255, 200,  50, 255};
    constexpr Color KNNPoint   = {255,  80,  80, 255}; // punto KNN
    constexpr Color MBRColor   = { 80, 180, 255,  60}; // MBR fill
    constexpr Color MBRBorder  = { 80, 180, 255, 180}; // MBR border
    constexpr Color LinearHit  = {255, 140,   0, 200}; // resultado lineal
}

enum class QueryMode { None, Range, KNN };

struct QueryStats {
    double rtreeTimeMs    = 0;
    double linearTimeMs   = 0;
    int    rtreeVisited   = 0;
    int    linearVisited  = 0;
    int    rtreeResults   = 0;
    int    linearResults  = 0;
};

struct MapView {
    MapCamera cam;
    RTree*    tree     = nullptr;
    Vector<SpatialObject>* objects = nullptr; // para busqueda lineal

    // estado de interaccion
    bool  dragging    = false;  // pan con click medio
    bool  selecting   = false;  // drag para rangeQuery
    float dragStartX  = 0, dragStartY  = 0;
    float selStartX   = 0, selStartY   = 0;
    float selEndX     = 0, selEndY     = 0;

    // resultados actuales
    Vector<Entry> rtreeResult;
    Vector<Entry> linearResult;
    QueryMode     currentMode = QueryMode::None;
    QueryStats    stats;

    // punto KNN
    float knnX = 0, knnY = 0;
    bool  hasKNN = false;
    int   knnK   = 5;

    // extras de visualizacion e interaccion
    bool showMBRs = true;    // dibujar los MBRs visitados en la ultima consulta
    int  nextId   = 900000;  // ids para puntos insertados a mano

    void init(RTree* t, Vector<SpatialObject>* objs) {
        tree    = t;
        objects = objs;
        cam.centerX = -60.0; // centrado en sudamerica
        cam.centerY = -20.0;
        cam.scale   = 4.0;
    }

    void handleEvent(const SDL_Event& e, float screenW, float screenH) {
        if (e.type == SDL_EVENT_MOUSE_WHEEL) {
            float factor = e.wheel.y > 0 ? 1.15f : 0.87f;
            cam.zoom(factor, e.wheel.mouse_x, e.wheel.mouse_y, screenW, screenH);
        }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            // flechas arriba/abajo: cambiar k del KNN
            if (e.key.key == SDLK_UP) {
                knnK++;
                if (hasKNN) runKNN(screenW, screenH);
            }
            if (e.key.key == SDLK_DOWN && knnK > 1) {
                knnK--;
                if (hasKNN) runKNN(screenW, screenH);
            }
            // M: mostrar/ocultar MBRs visitados
            if (e.key.key == SDLK_M)
                showMBRs = !showMBRs;
        }

        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            bool shift = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;

            // click medio: pan
            if (e.button.button == SDL_BUTTON_MIDDLE) {
                dragging   = true;
                dragStartX = e.button.x;
                dragStartY = e.button.y;
            }
            // shift + click izquierdo: insertar punto
            if (e.button.button == SDL_BUTTON_LEFT && shift) {
                insertAt(e.button.x, e.button.y, screenW, screenH);
            }
            // click izquierdo: inicio de rangeQuery
            else if (e.button.button == SDL_BUTTON_LEFT) {
                selecting  = true;
                selStartX  = selEndX = e.button.x;
                selStartY  = selEndY = e.button.y;
                rtreeResult  = Vector<Entry>();
                linearResult = Vector<Entry>();
                currentMode  = QueryMode::None;
            }
            // shift + click derecho: eliminar el punto mas cercano
            if (e.button.button == SDL_BUTTON_RIGHT && shift) {
                deleteNearest(e.button.x, e.button.y, screenW, screenH);
            }
            // click derecho: KNN
            else if (e.button.button == SDL_BUTTON_RIGHT) {
                knnX   = e.button.x;
                knnY   = e.button.y;
                hasKNN = true;
                rtreeResult  = Vector<Entry>();
                linearResult = Vector<Entry>();
                runKNN(screenW, screenH);
            }
        }

        if (e.type == SDL_EVENT_MOUSE_MOTION) {
            if (dragging) {
                cam.pan(e.motion.xrel, e.motion.yrel);
            }
            if (selecting) {
                selEndX = e.motion.x;
                selEndY = e.motion.y;
            }
        }

        if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            if (e.button.button == SDL_BUTTON_MIDDLE)
                dragging = false;

            if (e.button.button == SDL_BUTTON_LEFT && selecting) {
                selecting = false;
                // solo ejecuta si el drag fue suficientemente grande
                float dx = selEndX - selStartX;
                float dy = selEndY - selStartY;
                if (dx * dx + dy * dy > 25.0f)
                    runRangeQuery(screenW, screenH);
                else {
                    // click sin drag: limpiar seleccion
                    currentMode = QueryMode::None;
                    hasKNN      = false;
                }
            }
        }
    }

    void insertAt(float sx, float sy, float screenW, float screenH) {
        if (!tree || !objects) return;

        SpatialObject obj;
        obj.id       = nextId++;
        obj.name     = "Punto " + std::to_string(obj.id);
        obj.category = "custom";
        obj.x        = cam.toWorldX(sx, screenW);
        obj.y        = cam.toWorldY(sy, screenH);

        objects->push_back(obj);
        tree->insert(obj.id, obj.name, obj.x, obj.y, obj.category);
    }

    void deleteNearest(float sx, float sy, float screenW, float screenH) {
        if (!tree || !objects) return;

        double wx = cam.toWorldX(sx, screenW);
        double wy = cam.toWorldY(sy, screenH);

        // el vecino mas cercano al click es el que se elimina
        Vector<Entry> nearest = tree->knn(wx, wy, 1);
        if (nearest.empty()) return;

        const SpatialObject& victim = nearest[0].obj;
        if (!tree->remove(victim.id, victim.x, victim.y)) return;

        // tambien lo sacamos del vector usado por la busqueda lineal
        for (size_t i = 0; i < objects->size(); i++) {
            if ((*objects)[i].id == victim.id) {
                objects->erase(i);
                break;
            }
        }

        // limpiamos resultados que podrian referenciar al objeto borrado
        rtreeResult  = Vector<Entry>();
        linearResult = Vector<Entry>();
        currentMode  = QueryMode::None;
        hasKNN       = false;
    }

    void runRangeQuery(float screenW, float screenH) {
        if (!tree || !objects) return;
        currentMode = QueryMode::Range;
        hasKNN      = false;

        float x1 = std::min(selStartX, selEndX);
        float y1 = std::min(selStartY, selEndY);
        float x2 = std::max(selStartX, selEndX);
        float y2 = std::max(selStartY, selEndY);

        double wx1 = cam.toWorldX(x1, screenW);
        double wy1 = cam.toWorldY(y1, screenH); // y1 en pantalla = maxY en mundo
        double wx2 = cam.toWorldX(x2, screenW);
        double wy2 = cam.toWorldY(y2, screenH); // y2 en pantalla = minY en mundo

        double minX = std::min(wx1, wx2);
        double maxX = std::max(wx1, wx2);
        double minY = std::min(wy1, wy2);
        double maxY = std::max(wy1, wy2);

        // R-Tree
        auto t0 = std::chrono::high_resolution_clock::now();
        rtreeResult = tree->rangeQuery(minX, minY, maxX, maxY);
        auto t1 = std::chrono::high_resolution_clock::now();
        stats.rtreeTimeMs  = std::chrono::duration<double, std::milli>(t1 - t0).count();
        stats.rtreeResults = (int)rtreeResult.size();
        stats.rtreeVisited = tree->lastVisited;

        // busqueda lineal
        linearResult = Vector<Entry>();
        int visited  = 0;
        auto t2 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < objects->size(); i++) {
            visited++;
            const SpatialObject& obj = (*objects)[i];
            if (obj.x >= minX && obj.x <= maxX &&
                obj.y >= minY && obj.y <= maxY) {
                Entry e;
                e.obj = obj;
                e.mbr = {obj.x, obj.y, obj.x, obj.y};
                linearResult.push_back(e);
            }
        }
        auto t3 = std::chrono::high_resolution_clock::now();
        stats.linearTimeMs   = std::chrono::duration<double, std::milli>(t3 - t2).count();
        stats.linearVisited  = visited;
        stats.linearResults  = (int)linearResult.size();
    }

    void runKNN(float screenW, float screenH) {
        if (!tree || !objects) return;
        currentMode = QueryMode::KNN;

        double wx = cam.toWorldX(knnX, screenW);
        double wy = cam.toWorldY(knnY, screenH);

        // R-Tree KNN
        auto t0 = std::chrono::high_resolution_clock::now();
        rtreeResult = tree->knn(wx, wy, knnK);
        auto t1 = std::chrono::high_resolution_clock::now();
        stats.rtreeTimeMs  = std::chrono::duration<double, std::milli>(t1 - t0).count();
        stats.rtreeResults = (int)rtreeResult.size();
        stats.rtreeVisited = tree->lastVisited;

        // busqueda lineal KNN: ordenar por distancia y tomar k
        // usamos un vector simple y ordenamos al final
        Vector<std::pair<double, Entry>> all;
        int visited = 0;
        auto t2 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < objects->size(); i++) {
            visited++;
            const SpatialObject& obj = (*objects)[i];
            double dx = obj.x - wx;
            double dy = obj.y - wy;
            double d  = std::sqrt(dx*dx + dy*dy);
            Entry e;
            e.obj = obj;
            e.mbr = {obj.x, obj.y, obj.x, obj.y};
            all.push_back({d, e});
        }
        // insertion sort para los k menores (k es pequeño)
        for (int i = 1; i < (int)all.size(); i++) {
            auto key = all[i];
            int j = i - 1;
            while (j >= 0 && all[j].first > key.first) {
                all[j + 1] = all[j];
                j--;
            }
            all[j + 1] = key;
        }
        auto t3 = std::chrono::high_resolution_clock::now();
        stats.linearTimeMs  = std::chrono::duration<double, std::milli>(t3 - t2).count();
        stats.linearVisited = visited;

        linearResult = Vector<Entry>();
        for (int i = 0; i < knnK && i < (int)all.size(); i++)
            linearResult.push_back(all[i].second);
        stats.linearResults = (int)linearResult.size();
    }

    void render(Window& win, float screenW, float screenH) {
        // MBRs de los nodos visitados en la ultima consulta
        if (showMBRs && tree && currentMode != QueryMode::None) {
            for (size_t i = 0; i < tree->visitedMBRs.size(); i++) {
                const MBR& m = tree->visitedMBRs[i];
                float x1 = cam.toScreenX(m.minX, screenW);
                float y1 = cam.toScreenY(m.maxY, screenH); // maxY arriba
                float x2 = cam.toScreenX(m.maxX, screenW);
                float y2 = cam.toScreenY(m.minY, screenH);
                if (x2 < 0 || x1 > screenW || y2 < 0 || y1 > screenH)
                    continue; // fuera de pantalla
                Rect r = {x1, y1, x2 - x1, y2 - y1};
                win.DrawRect(r, MapColors::MBRColor, MapColors::MBRBorder, 1);
            }
        }

        // puntos base
        for (size_t i = 0; i < objects->size(); i++) {
            const SpatialObject& obj = (*objects)[i];
            float sx = cam.toScreenX(obj.x, screenW);
            float sy = cam.toScreenY(obj.y, screenH);
            if (sx < -5 || sx > screenW + 5 || sy < -5 || sy > screenH + 5)
                continue; // fuera de pantalla, no dibujar

            Color c = (obj.category == "heladeria") ? MapColors::Heladeria
                    : (obj.category == "custom")    ? MapColors::Custom
                                                    : MapColors::City;
            Rect r = {sx - 2, sy - 2, 4, 4};
            win.DrawRect(r, c, c, 0);
        }

        // rectangulo de seleccion mientras se arrastra
        if (selecting) {
            float x1 = std::min(selStartX, selEndX);
            float y1 = std::min(selStartY, selEndY);
            float w  = std::abs(selEndX - selStartX);
            float h  = std::abs(selEndY - selStartY);
            Rect sel = {x1, y1, w, h};
            win.DrawRect(sel, MapColors::QueryRect, MapColors::QueryBorder, 1);
        }

        // resultados rangeQuery
        if (currentMode == QueryMode::Range) {
            // rectangulo de consulta final
            float x1 = std::min(selStartX, selEndX);
            float y1 = std::min(selStartY, selEndY);
            float w  = std::abs(selEndX - selStartX);
            float h  = std::abs(selEndY - selStartY);
            Rect sel = {x1, y1, w, h};
            win.DrawRect(sel, MapColors::QueryRect, MapColors::QueryBorder, 2);

            // resultados del rtree
            for (size_t i = 0; i < rtreeResult.size(); i++) {
                const SpatialObject& obj = rtreeResult[i].obj;
                float sx = cam.toScreenX(obj.x, screenW);
                float sy = cam.toScreenY(obj.y, screenH);
                Color c = (obj.category == "heladeria") ? MapColors::ResultHel
                                                        : MapColors::ResultCity;
                Rect r = {sx - 4, sy - 4, 8, 8};
                win.DrawRect(r, c, c, 0);
            }
        }

        // resultados KNN
        if (currentMode == QueryMode::KNN && hasKNN) {
            // punto de consulta
            Rect qr = {knnX - 6, knnY - 6, 12, 12};
            win.DrawRect(qr, MapColors::KNNPoint, MapColors::KNNPoint, 0);

            // lineas desde el punto a cada vecino
            for (size_t i = 0; i < rtreeResult.size(); i++) {
                const SpatialObject& obj = rtreeResult[i].obj;
                float sx = cam.toScreenX(obj.x, screenW);
                float sy = cam.toScreenY(obj.y, screenH);
                win.DrawLine(knnX, knnY, sx, sy, MapColors::KNNPoint);
                Rect r = {sx - 5, sy - 5, 10, 10};
                Color c = (obj.category == "heladeria") ? MapColors::ResultHel
                                                        : MapColors::ResultCity;
                win.DrawRect(r, c, c, 0);
            }
        }
    }

    // formatea un double con 3 decimales (los tiempos suelen ser < 1 ms)
    static std::string fmtMs(double v) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.3f", v);
        return std::string(buf);
    }

    // panel de stats para dibujar aparte
    void renderStats(Window& win, float x, float y) {
        if (currentMode == QueryMode::None) return;

        auto line = [&](const std::string& txt, float dy) {
            win.DrawText(txt, x, y + dy, Colors::White, 14);
        };

        std::string mode = currentMode == QueryMode::Range ? "Range Query" : "KNN (k=" + std::to_string(knnK) + ")";
        line("[ " + mode + " ]", 0);
        line("R-Tree:  " + fmtMs(stats.rtreeTimeMs)  + " ms  |  " +
             std::to_string(stats.rtreeResults) + " resultados", 20);
        line("Lineal:  " + fmtMs(stats.linearTimeMs) + " ms  |  " +
             std::to_string(stats.linearResults) + " resultados", 40);
        line("Nodos visitados R-Tree: " + std::to_string(stats.rtreeVisited) +
             "  |  Elementos lineal: " + std::to_string(stats.linearVisited), 60);
    }
};