#pragma once

#include "../include/rtree.hpp"
#include "../include/vector.hpp"

#include <fstream>
#include <sstream>
#include <string>

#include <SDL3/SDL.h>

// Carga los primeros maxCount objetos del archivo de GeoNames
// Formato: TSV sin encabezado, columnas relevantes:
//   0: geonameid, 1: name, 4: latitude, 5: longitude, 8: country code
inline Vector<SpatialObject> loadGeoNames(const std::string &path,
                                          int maxCount = -1) {
  Vector<SpatialObject> result;
  std::ifstream file(path);

  if (!file.is_open()) {
    SDL_Log("DataLoader: no se pudo abrir %s", path.c_str());
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

    int id = 0;
    double lat = 0, lon = 0;
    try {
      id = std::stoi(cols[0]);
      lat = std::stod(cols[4]);
      lon = std::stod(cols[5]);
    } catch (...) {
      continue;
    }

    SpatialObject obj;
    obj.id = id;
    obj.name = cols[1];
    obj.category = cols[8];
    obj.x = lon;
    obj.y = lat;
    result.push_back(obj);
    count++;
  }

  return result;
}

inline Vector<SpatialObject> loadOverpassJSON(const std::string &path,
                                              int startId = 200000) {
  Vector<SpatialObject> result;
  std::ifstream file(path);

  if (!file.is_open()) {
    SDL_Log("DataLoader: no se pudo abrir %s", path.c_str());
    return result;
  }

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  size_t pos = 0;
  int autoId = startId;

  while ((pos = content.find("\"type\": \"node\"", pos)) != std::string::npos) {
    size_t blockEnd = content.find("\"type\": \"node\"", pos + 1);
    if (blockEnd == std::string::npos)
      blockEnd = content.size();
    std::string block = content.substr(pos, blockEnd - pos);

    // extrae un campo del bloque, maneja strings y numeros
    auto extractField = [&](const std::string &key) -> std::string {
      std::string search = "\"" + key + "\":";
      size_t p = block.find(search);
      if (p == std::string::npos)
        return "";
      p += search.size();
      while (p < block.size() && block[p] == ' ')
        p++;
      if (p >= block.size())
        return "";

      if (block[p] == '"') {
        p++;
        size_t end = block.find('"', p);
        return end != std::string::npos ? block.substr(p, end - p) : "";
      } else {
        size_t end = p;
        while (end < block.size() && (std::isdigit(block[end]) ||
                                      block[end] == '.' || block[end] == '-'))
          end++;
        return block.substr(p, end - p);
      }
    };

    std::string latStr = extractField("lat");
    std::string lonStr = extractField("lon");
    std::string name = extractField("name");

    if (latStr.empty() || lonStr.empty()) {
      pos++;
      continue;
    }

    try {
      SpatialObject obj;
      obj.id = autoId++;
      obj.name = name.empty() ? "Grido" : name;
      obj.category = "heladeria";
      obj.y = std::stod(latStr);
      obj.x = std::stod(lonStr);
      result.push_back(obj);
    } catch (...) {
    }

    pos++;
  }

  return result;
}

// Inserta un vector de SpatialObjects en el RTree
inline void loadIntoRTree(RTree &tree, const Vector<SpatialObject> &objects) {
  for (size_t i = 0; i < objects.size(); i++) {
    const SpatialObject &obj = objects[i];
    tree.insert(obj.id, obj.name, obj.x, obj.y, obj.category);
  }
}
