#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

#include "Widgets.hpp"

class Window {
public:
  bool Init(const char *title, int w, int h) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      SDL_Log("SDL_Init failed: %s", SDL_GetError());
      return false;
    }
    if (!TTF_Init()) {
      SDL_Log("TTF_Init failed: %s", SDL_GetError());
      return false;
    }

    m_window = SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE);
    if (!m_window) {
      SDL_Log("CreateWindow failed: %s", SDL_GetError());
      return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
      SDL_Log("CreateRenderer failed: %s", SDL_GetError());
      return false;
    }

    SDL_SetRenderVSync(m_renderer, 1);
    SDL_StartTextInput(m_window);
    return true;
  }

  void Quit() {
    for (auto &[sz, font] : m_fonts)
      TTF_CloseFont(font);
    m_fonts.clear();

    if (m_renderer)
      SDL_DestroyRenderer(m_renderer);
    if (m_window)
      SDL_DestroyWindow(m_window);
    TTF_Quit();
    SDL_Quit();
  }

  bool LoadFont(const char *path, int size = 16) {
    TTF_Font *f = TTF_OpenFont(path, (float)size);
    if (!f) {
      SDL_Log("TTF_OpenFont failed: %s", SDL_GetError());
      return false;
    }
    m_fonts[size] = f;
    m_defaultSize = size;
    return true;
  }

  void SetIcon(const char *path) {
    SDL_Surface *icon = IMG_Load(path);
    if (!icon) {
      SDL_Log("SetIcon: IMG_Load failed: %s", SDL_GetError());
      return;
    }
    SDL_SetWindowIcon(m_window, icon);
    SDL_DestroySurface(icon);
  }

  void BeginFrame(Color bg = Colors::LightGray) {
    SDL_SetRenderDrawColor(m_renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderClear(m_renderer);
  }

  void EndFrame() { SDL_RenderPresent(m_renderer); }

  void DrawRect(const Rect &r, Color fill, Color border = Colors::MidGray,
                float borderW = 1.0f) {
    SetColor(fill);
    SDL_RenderFillRect(m_renderer, &r);
    if (borderW > 0) {
      SetColor(border);
      SDL_RenderRect(m_renderer, &r);
    }
  }

  void DrawRectAlpha(const Rect &r, Color fill, Color border,
                     float borderW = 1.0f) {
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SetColor(fill);
    SDL_RenderFillRect(m_renderer, &r);
    if (borderW > 0) {
      SetColor(border);
      SDL_RenderRect(m_renderer, &r);
    }
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
  }

  void DrawBorder(const Rect &r, Color c, float thickness = 2.f) {
    SetColor(c);
    for (float i = 0; i < thickness; ++i) {
      Rect inner = {r.x + i, r.y + i, std::max(0.f, r.w - 2.f * i),
                    std::max(0.f, r.h - 2.f * i)};
      SDL_RenderRect(m_renderer, &inner);
    }
  }

  void DrawLine(float x1, float y1, float x2, float y2, Color c) {
    SetColor(c);
    SDL_RenderLine(m_renderer, x1, y1, x2, y2);
  }

  void DrawText(const std::string &text, float x, float y, Color fg,
                int fontSize = 0, const Rect *clip = nullptr) {
    if (text.empty())
      return;
    TTF_Font *font = GetFont(fontSize ? fontSize : m_defaultSize);
    if (!font)
      return;

    SDL_Color c = {fg.r, fg.g, fg.b, fg.a};
    SDL_Surface *surf =
        TTF_RenderText_Blended(font, text.c_str(), text.size(), c);
    if (!surf)
      return;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(m_renderer, surf);
    SDL_DestroySurface(surf);
    if (!tex)
      return;

    float tw, th;
    SDL_GetTextureSize(tex, &tw, &th);
    Rect dst = {x, y, tw, th};

    if (clip && clip->w > 0) {
      SDL_Rect clp;
      clp.x = (int)clip->x;
      clp.y = (int)clip->y;
      clp.w = (int)clip->w;
      clp.h = (int)clip->h;
      SDL_SetRenderClipRect(m_renderer, &clp);
    }
    SDL_RenderTexture(m_renderer, tex, nullptr, &dst);
    if (clip && clip->w > 0) {
      SDL_SetRenderClipRect(m_renderer, nullptr);
    }
    SDL_DestroyTexture(tex);
  }

  void DrawTextCentered(const std::string &text, const Rect &r, Color fg,
                        int fontSize = 0) {
    TTF_Font *font = GetFont(fontSize ? fontSize : m_defaultSize);
    if (!font || text.empty())
      return;

    int tw, th;
    TTF_GetStringSize(font, text.c_str(), text.size(), &tw, &th);
    float x = r.x + (r.w - (float)tw) * 0.5f;
    float y = r.y + (r.h - (float)th) * 0.5f;
    DrawText(text, x, y, fg, fontSize ? fontSize : m_defaultSize);
  }

  void Render(const TextBox &tb) {
    Color border = tb.selected ? Colors::Blue : tb.border;
    DrawRect(tb.bounds, tb.bg, border, tb.selected ? 2.0f : 1.0f);
    if (!tb.text.empty()) {
      Rect clip = {tb.bounds.x + 3, tb.bounds.y, tb.bounds.w - 6, tb.bounds.h};
      DrawText(tb.text, tb.bounds.x + 4, tb.bounds.y + 4, tb.fg, tb.fontSize,
               &clip);
    }
  }

  void Render(const InputBox &ib) {
    Color border = ib.active ? Colors::Blue : ib.border;
    DrawRect(ib.bounds, ib.bg, border, ib.active ? 2.0f : 1.0f);

    const std::string &display = ib.text.empty() ? ib.placeholder : ib.text;
    Color fg = ib.text.empty() ? Colors::MidGray : ib.fg;
    if (!display.empty()) {
      Rect clip = {ib.bounds.x + 3, ib.bounds.y, ib.bounds.w - 6, ib.bounds.h};
      DrawText(display, ib.bounds.x + 4, ib.bounds.y + 4, fg, ib.fontSize,
               &clip);
    }

    if (ib.active) {
      TTF_Font *font = GetFont(ib.fontSize);
      int tw = 0;
      if (font && !ib.text.empty())
        TTF_GetStringSize(font, ib.text.c_str(), ib.text.size(), &tw, nullptr);
      float cx = ib.bounds.x + 4 + (float)tw;
      float cy = ib.bounds.y + 4;
      float ch = ib.bounds.h - 8;
      if ((SDL_GetTicks() / 500) % 2 == 0)
        DrawLine(cx, cy, cx, cy + ch, ib.fg);
    }
  }

  void Render(const Button &btn) {
    DrawRect(btn.bounds, btn.bg, btn.border, 1.0f);
    DrawTextCentered(btn.label, btn.bounds, btn.fg, btn.fontSize);
  }

  SDL_Renderer *GetRenderer() const { return m_renderer; }
  SDL_Window *GetWindow() const { return m_window; }

  int Width() const {
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    return w;
  }
  int Height() const {
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    return h;
  }

private:
  SDL_Window *m_window = nullptr;
  SDL_Renderer *m_renderer = nullptr;

  std::unordered_map<int, TTF_Font *> m_fonts;
  int m_defaultSize = 16;

  TTF_Font *GetFont(int size) {
    auto it = m_fonts.find(size);
    if (it != m_fonts.end())
      return it->second;
    it = m_fonts.find(m_defaultSize);
    return it != m_fonts.end() ? it->second : nullptr;
  }

  void SetColor(Color c) {
    SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
  }
};

#endif // WINDOW_HPP