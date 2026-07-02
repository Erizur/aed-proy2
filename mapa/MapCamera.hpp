#pragma once

struct MapCamera {
    double centerX = 0, centerY = 0; // centro del mundo visible
    double scale   = 3.0;            // pixeles por grado

    // mundo -> pantalla
    float toScreenX(double wx, float screenW) const {
        return (float)((wx - centerX) * scale + screenW / 2.0);
    }
    float toScreenY(double wy, float screenH) const {
        // Y invertido: latitud sube, pantalla baja
        return (float)(-(wy - centerY) * scale + screenH / 2.0);
    }

    // pantalla -> mundo
    double toWorldX(float sx, float screenW) const {
        return (sx - screenW / 2.0) / scale + centerX;
    }
    double toWorldY(float sy, float screenH) const {
        return -(sy - screenH / 2.0) / scale + centerY;
    }

    // zoom centrado en el cursor
    void zoom(float factor, float mx, float my, float screenW, float screenH) {
        double wx = toWorldX(mx, screenW);
        double wy = toWorldY(my, screenH);
        scale *= factor;
        centerX = wx - (mx - screenW / 2.0) / scale;
        centerY = wy + (my - screenH / 2.0) / scale;
    }

    void pan(float dx, float dy) {
        centerX -= dx / scale;
        centerY += dy / scale;
    }
};