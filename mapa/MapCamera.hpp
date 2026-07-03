#pragma once

#include <algorithm>
#include <cmath>

// Camara con proyeccion Mercator para coincidir con OSM tiles
struct MapCamera {
  double centerLon = 0;   // longitud del centro (grados)
  double centerMercY = 0; // Y Mercator del centro (radianes)
  double scale = 200.0;   // pixeles por radian Mercator

  static double latToMerc(double lat) {
    double r = lat * M_PI / 180.0;
    return std::log(std::tan(M_PI / 4.0 + r / 2.0));
  }
  static double mercToLat(double y) {
    return 180.0 / M_PI * (2.0 * std::atan(std::exp(y)) - M_PI / 2.0);
  }

  // helper para inicializar con lat/lon normales
  void setCenter(double lat, double lon) {
    centerLon = lon;
    centerMercY = latToMerc(lat);
  }

  // mundo (lon,lat) -> pantalla
  float toScreenX(double lon, float screenW) const {
    double dLonRad = (lon - centerLon) * M_PI / 180.0;
    return (float)(dLonRad * scale + screenW / 2.0);
  }
  float toScreenY(double lat, float screenH) const {
    double m = latToMerc(lat);
    return (float)(-(m - centerMercY) * scale + screenH / 2.0);
  }

  // pantalla -> mundo (lon,lat)
  double toWorldX(float sx, float screenW) const {
    double dLonRad = (sx - screenW / 2.0) / scale;
    return centerLon + dLonRad * 180.0 / M_PI;
  }
  double toWorldY(float sy, float screenH) const {
    double dm = -(sy - screenH / 2.0) / scale;
    return mercToLat(centerMercY + dm);
  }

  void applyBounds(float screenW, float screenH) {
    centerLon = std::max(-180.0, std::min(180.0, centerLon));
    double maxM = latToMerc(85.05);
    double halfMerc = (screenH / 2.0) / scale;
    double maxCenter = maxM - halfMerc;
    double minCenter = -maxM + halfMerc;
    if (maxCenter < minCenter) {
      centerMercY = 0;
    } else {
      centerMercY = std::max(minCenter, std::min(maxCenter, centerMercY));
    }
  }

  void pan(float dx, float dy, float screenW, float screenH) {
    double dLonRad = -dx / scale;
    centerLon += dLonRad * 180.0 / M_PI;
    centerMercY += dy / scale;
    applyBounds(screenW, screenH);
  }

  void zoom(float factor, float mx, float my, float screenW, float screenH) {
    // guardar coords mundo bajo el cursor antes del zoom
    double wxLon = toWorldX(mx, screenW);
    double wyLat = toWorldY(my, screenH);
    double wyMerc = latToMerc(wyLat);

    scale *= factor;
    // minScale: mundo entero cabe en la pantalla (2π radianes)
    double minScale = screenW / (2.0 * M_PI);
    scale = std::max(minScale, std::min(scale, 10000000.0));

    // reposicionar para que el cursor apunte al mismo punto del mundo
    centerLon = wxLon - (mx - screenW / 2.0) / scale * 180.0 / M_PI;
    centerMercY = wyMerc + (my - screenH / 2.0) / scale;
    applyBounds(screenW, screenH);
  }
};