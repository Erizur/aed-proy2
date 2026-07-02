#pragma once
#include <SDL3/SDL.h>

using Color = SDL_Color;

#define rgb(a, b, c) {a, b, c, 255}

namespace Colors {
// basic
constexpr Color White rgb(255, 255, 255);
constexpr Color Black rgb(0, 0, 0);
constexpr Color LightGray rgb(220, 220, 220);
constexpr Color MidGray rgb(160, 160, 160);
constexpr Color DarkGray rgb(60, 60, 60);
constexpr Color Blue rgb(66, 135, 245);
constexpr Color LightBlue rgb(210, 230, 255);
constexpr Color Red rgb(220, 50, 50);
constexpr Color Green rgb(50, 180, 80);
constexpr Color Yellow rgb(255, 220, 50);

// grid
constexpr Color CellBg rgb(255, 255, 255);
constexpr Color CellSelected rgb(195, 215, 255);
constexpr Color CellOccupied rgb(232, 245, 233);
constexpr Color CellRangeHL rgb(204, 220, 255);   // range selection fill
constexpr Color SelectionBorder rgb(30, 90, 210); // thick border around range
constexpr Color CellEditing rgb(255, 255, 230);
constexpr Color GridLine rgb(200, 200, 200);
constexpr Color HeaderBg rgb(75, 85, 100);
constexpr Color HeaderFg rgb(240, 240, 245);
constexpr Color HeaderActive rgb(55, 100, 170);

// tb / status
constexpr Color ToolbarBg rgb(245, 245, 248);
constexpr Color StatusBg rgb(55, 65, 80);
constexpr Color StatusFg rgb(210, 215, 225);
constexpr Color AccentGreen rgb(46, 160, 67);
constexpr Color AccentRed rgb(200, 60, 60);
constexpr Color AccentOrange rgb(220, 150, 40);
constexpr Color BtnPrimary rgb(56, 120, 220);
constexpr Color BtnPrimaryFg rgb(255, 255, 255);
constexpr Color BtnDanger rgb(200, 55, 55);
constexpr Color BtnDangerFg rgb(255, 255, 255);
} // namespace Colors

using Rect = SDL_FRect;

inline Rect MakeRect(float x, float y, float w, float h) {
  return {x, y, w, h};
}