#pragma once

#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <curl/curl.h>

// buffer para curl
struct CurlBuffer {
  std::string data;
};

static size_t curlWrite(void *ptr, size_t size, size_t nmemb, void *userdata) {
  auto *buf = static_cast<CurlBuffer *>(userdata);
  buf->data.append((char *)ptr, size * nmemb);
  return size * nmemb;
}

struct TileKey {
  int z, x, y;
  bool operator==(const TileKey &o) const {
    return z == o.z && x == o.x && y == o.y;
  }
};

struct TileKeyHash {
  size_t operator()(const TileKey &k) const {
    return std::hash<int>()(k.z) ^ (std::hash<int>()(k.x) << 8) ^
           (std::hash<int>()(k.y) << 16);
  }
};

struct TileCache {
  SDL_Renderer *renderer = nullptr;
  std::string cacheDir;

  std::unordered_map<TileKey, SDL_Texture *, TileKeyHash> loaded;
  std::unordered_map<TileKey, bool, TileKeyHash> pending;
  std::queue<TileKey> downloadQueue;

  std::mutex mutex;
  SDL_Thread *thread = nullptr;
  bool running = false;

  // tiles descargados listos para subir a GPU
  struct ReadyTile {
    TileKey key;
    SDL_Surface *surf;
  };
  std::vector<ReadyTile> readyQueue;
  std::mutex readyMutex;

  void init(SDL_Renderer *r) {
    renderer = r;
    cacheDir = "./tiles/";
    running = true;
    thread = SDL_CreateThread(threadFunc, "TileDownloader", this);

    // warm up desde zoom 2 — 4 tiles que cubren el mundo sin distorsion visible
    get(0, 0, 0);
    get(1, 0, 0);
    get(1, 1, 0);
  }

  void quit() {
    running = false;
    SDL_WaitThread(thread, nullptr);
    for (auto &[k, tex] : loaded)
      if (tex)
        SDL_DestroyTexture(tex);
    loaded.clear();
  }

  // llamar cada frame desde el hilo principal
  void uploadReady() {
    std::lock_guard<std::mutex> lock(readyMutex);
    for (auto &rt : readyQueue) {
      SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, rt.surf);
      SDL_DestroySurface(rt.surf);
      if (tex) {
        std::lock_guard<std::mutex> l2(mutex);
        loaded[rt.key] = tex;
        pending.erase(rt.key);
      }
    }
    readyQueue.clear();
  }

  // devuelve la textura si esta lista, nullptr si no (y la encola)
  SDL_Texture *get(int z, int x, int y) {
    TileKey key{z, x, y};
    {
      std::lock_guard<std::mutex> lock(mutex);
      auto it = loaded.find(key);
      if (it != loaded.end())
        return it->second;
      if (pending.count(key))
        return nullptr;
      // limitar la cola para no spamear OSM al hacer zoom rapido
      if (downloadQueue.size() < 32) {
        pending[key] = true;
        downloadQueue.push(key);
      }
    }
    return nullptr;
  }

  // solo consulta, no encola — para fallback de zoom anterior
  SDL_Texture *getIfLoaded(int z, int x, int y) {
    TileKey key{z, x, y};
    std::lock_guard<std::mutex> lock(mutex);
    auto it = loaded.find(key);
    return it != loaded.end() ? it->second : nullptr;
  }

private:
  static int threadFunc(void *data) {
    auto *self = static_cast<TileCache *>(data);
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "AED-Proyecto2/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    while (self->running) {
      TileKey key{0, 0, 0};
      bool hasWork = false;
      {
        std::lock_guard<std::mutex> lock(self->mutex);
        if (!self->downloadQueue.empty()) {
          key = self->downloadQueue.front();
          self->downloadQueue.pop();
          hasWork = true;
        }
      }

      if (!hasWork) {
        SDL_Delay(10);
        continue;
      }

      // ruta en disco
      std::string path = self->cacheDir + std::to_string(key.z) + "/" +
                         std::to_string(key.x) + "/" + std::to_string(key.y) +
                         ".png";

      SDL_Surface *surf = nullptr;

      // intentar cargar desde disco primero
      surf = IMG_Load(path.c_str());

      if (!surf) {
        // descargar
        std::string url = "https://tile.openstreetmap.org/" +
                          std::to_string(key.z) + "/" + std::to_string(key.x) +
                          "/" + std::to_string(key.y) + ".png";

        CurlBuffer buf;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
        CURLcode res = curl_easy_perform(curl);

        if (res == CURLE_OK && !buf.data.empty()) {
          // guardar en disco
          SDL_CreateDirectory((self->cacheDir + std::to_string(key.z) + "/" +
                               std::to_string(key.x) + "/")
                                  .c_str());

          FILE *f = fopen(path.c_str(), "wb");
          if (f) {
            fwrite(buf.data.data(), 1, buf.data.size(), f);
            fclose(f);
          }

          // cargar como surface desde memoria
          SDL_IOStream *io =
              SDL_IOFromMem((void *)buf.data.data(), (int)buf.data.size());
          surf = IMG_LoadPNG_IO(io);
          SDL_CloseIO(io);
        }
      }

      if (surf) {
        std::lock_guard<std::mutex> lock(self->readyMutex);
        self->readyQueue.push_back({key, surf});
      } else {
        // marcar como fallido para no reintentar
        std::lock_guard<std::mutex> lock(self->mutex);
        self->pending.erase(key);
      }

      SDL_Delay(50); // rate limiting basico
    }

    curl_easy_cleanup(curl);
    return 0;
  }
};