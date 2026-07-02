#pragma once

#include <functional>
#include <string>

#include "Defines.hpp"

struct TextBox {
  Rect bounds;
  std::string text;
  Color bg = Colors::White;
  Color fg = Colors::Black;
  Color border = Colors::MidGray;
  bool selected = false;
  int fontSize = 16;
};

struct InputBox {
  Rect bounds;
  std::string text;
  std::string placeholder;
  bool active = false;
  Color bg = Colors::White;
  Color fg = Colors::Black;
  Color border = Colors::MidGray;
  int fontSize = 16;

  bool HandleEvent(const SDL_Event &e) {
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
      float mx = e.button.x, my = e.button.y;
      active = (mx >= bounds.x && mx <= bounds.x + bounds.w && my >= bounds.y &&
                my <= bounds.y + bounds.h);
      return false;
    }
    if (!active)
      return false;

    if (e.type == SDL_EVENT_TEXT_INPUT) {
      text += e.text.text;
      return false;
    }
    if (e.type == SDL_EVENT_KEY_DOWN) {
      switch (e.key.key) {
      case SDLK_BACKSPACE:
        if (!text.empty())
          text.pop_back();
        break;
      case SDLK_ESCAPE:
        active = false;
        break;
      case SDLK_RETURN:
      case SDLK_KP_ENTER:
        active = false;
        return true;
      default:
        break;
      }
    }
    return false;
  }

  void Clear() { text.clear(); }
};

struct Button {
  Rect bounds;
  std::string label;
  Color bg = Colors::LightGray;
  Color fg = Colors::Black;
  Color border = Colors::MidGray;
  int fontSize = 16;
  std::function<void()> onClick;

  bool HandleEvent(const SDL_Event &e) {
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        e.button.button == SDL_BUTTON_LEFT) {
      float mx = e.button.x, my = e.button.y;
      if (mx >= bounds.x && mx <= bounds.x + bounds.w && my >= bounds.y &&
          my <= bounds.y + bounds.h) {
        if (onClick)
          onClick();
        return true;
      }
    }
    return false;
  }
};