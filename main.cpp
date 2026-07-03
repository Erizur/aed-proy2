#include "Defines.hpp"
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

#include <string>

#include "DataLoader.hpp"
#include "MapCamera.hpp"
#include "MapView.hpp"
#include "Window.hpp"

struct AppState {
  Window win;
  TileCache tileCache;
  RTree tree{8}; // maxSize 8 para 15k+ puntos
  Vector<SpatialObject> objects;
  MapView mapView;
  bool loaded = false;
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
  AppState *app = new AppState();

  if (!app->win.Init("R-Tree Spatial Explorer", 1280, 720)) {
    delete app;
    return SDL_APP_FAILURE;
  }

  // font
  bool fontLoaded = false;
#ifdef ASSET_DATA
  fontLoaded =
      app->win.LoadFont((std::string(ASSET_DATA) + "font.ttf").c_str(), 16);
  app->win.LoadFont((std::string(ASSET_DATA) + "font.ttf").c_str(), 14);
#endif
  if (!fontLoaded) {
    SDL_Log("No se pudo cargar font.ttf");
    delete app;
    return SDL_APP_FAILURE;
  }

  // cargar datasets
#ifdef ASSET_DATA
  std::string assetDir = std::string(ASSET_DATA);
#else
  std::string assetDir = std::string(SDL_GetBasePath()) + "assets/";
#endif

#ifdef ASSET_DATA
  app->win.SetIcon((std::string(ASSET_DATA) + "aed.png").c_str());
#else
  const char *sdlBase = SDL_GetBasePath();
  std::string base = sdlBase ? sdlBase : "./";
  auto tryIcon = [&](const std::string &dir) {
    app->win.SetIcon((dir + "assets/aed.png").c_str());
  };
  tryIcon(base);
  tryIcon(base + "../");
  tryIcon("./");
#endif

  SDL_Texture *pointSprite = nullptr;
  SDL_Texture *gridoSprite = nullptr;

  SDL_Surface *s1 = IMG_Load((assetDir + "normal.png").c_str());
  if (s1) {
    pointSprite = SDL_CreateTextureFromSurface(app->win.GetRenderer(), s1);
    SDL_DestroySurface(s1);
  } else {
    SDL_Log("No se pudo cargar normal.png");
  }

  SDL_Surface *s2 = IMG_Load((assetDir + "grido-dance.png").c_str());
  if (s2) {
    gridoSprite = SDL_CreateTextureFromSurface(app->win.GetRenderer(), s2);
    SDL_DestroySurface(s2);
  } else {
    SDL_Log("No se pudo cargar grido-dance.png");
  }

  app->mapView.setSprites(pointSprite, gridoSprite);

  SDL_Log("Cargando cities1000...");
  Vector<SpatialObject> cities = loadGeoNames(assetDir + "cities1000.txt", -1);
  SDL_Log("Ciudades cargadas: %d", (int)cities.size());

  SDL_Log("Cargando grido.json...");
  Vector<SpatialObject> gridos = loadOverpassJSON(assetDir + "grido.json");
  SDL_Log("Gridos cargados: %d", (int)gridos.size());

  // combinar en objects
  for (size_t i = 0; i < cities.size(); i++)
    app->objects.push_back(cities[i]);
  for (size_t i = 0; i < gridos.size(); i++)
    app->objects.push_back(gridos[i]);

  SDL_Log("Total objetos: %d — insertando en R-Tree...",
          (int)app->objects.size());
  loadIntoRTree(app->tree, app->objects);
  SDL_Log("R-Tree listo.");

  app->mapView.init(&app->tree, &app->objects);
  app->loaded = true;

  app->tileCache.init(app->win.GetRenderer());
  app->tileCache.uploadReady();
  app->mapView.renderTiles(app->win, app->tileCache, (float)app->win.Width(),
                           (float)app->win.Height());

  *appstate = app;
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  AppState *app = static_cast<AppState *>(appstate);

  if (event->type == SDL_EVENT_QUIT)
    return SDL_APP_SUCCESS;

  if (app->loaded) {
    float w = (float)app->win.Width();
    float h = (float)app->win.Height();
    app->mapView.handleEvent(*event, w, h);
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  AppState *app = static_cast<AppState *>(appstate);

  float w = (float)app->win.Width();
  float h = (float)app->win.Height();

  app->win.BeginFrame({170, 211, 223, 255}); // fondo oscuro tipo mapa

  if (app->loaded) {
    app->tileCache.uploadReady();

    app->mapView.renderTiles(app->win, app->tileCache, w, h);
    app->mapView.renderMBRs(app->win, w, h);
    app->mapView.render(app->win, w, h);

    // panel de stats en esquina inferior izquierda
    Rect statsBg = {10, h - 120, 460, 110};
    app->win.DrawRect(statsBg, {0, 0, 0, 160}, {80, 80, 80, 200}, 1);
    app->mapView.renderStats(app->win, 18, h - 112);

    // leyenda esquina superior derecha
    float lx = w - 160;
    app->win.DrawText("● Ciudades", lx, 12, MapColors::City, 14);
    app->win.DrawText("● Grido", lx, 30, MapColors::Heladeria, 14);
    app->win.DrawText("● Resultado", lx, 48, MapColors::ResultCity, 14);
    app->win.DrawText("● Insertado", lx, 66, MapColors::Custom, 14);
    app->win.DrawText("□ MBR visitado", lx, 84, MapColors::MBRBorder, 14);

    // instrucciones esquina superior izquierda
    app->win.DrawText("Drag izq: Range Query", 10, 12, Colors::DarkGray, 14);
    app->win.DrawText("Click der: KNN", 10, 30, Colors::DarkGray, 14);
    app->win.DrawText("Shift+Click izq: Insertar", 10, 48, Colors::DarkGray, 14);
    app->win.DrawText("Shift+Click der: Eliminar", 10, 66, Colors::DarkGray, 14);
    app->win.DrawText("Flechas arr/abj: cambiar k", 10, 84, Colors::DarkGray,
                      14);
    app->win.DrawText("M: MBRs on/off", 10, 102, Colors::DarkGray, 14);
    app->win.DrawText("Scroll: Zoom | Click medio: Pan", 10, 120,
                      Colors::DarkGray, 14);
    app->win.DrawText("Espacio: baile", 10, 138, Colors::DarkGray, 14);
  }

  app->win.EndFrame();
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  AppState *app = static_cast<AppState *>(appstate);
  if (!app)
    return;
  app->tileCache.quit();
  app->win.Quit();
  delete app;
}
