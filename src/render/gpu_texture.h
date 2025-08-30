#pragma once

#include <SDL3/SDL_gpu.h>
#include <string>

enum class TextureType {
  SAMPLER,
  TARGET,
  DEPTH,
  STORAGE,
};

struct GPUTexture {
private:
  SDL_GPUDevice *device = nullptr;
  SDL_GPUTexture *texture = nullptr;

#ifdef PRJ_DEBUG
  std::string name;
#endif

public:
  bool init(
    SDL_GPUDevice *device,
    uint32_t w,
    uint32_t h,
    SDL_PixelFormat format,
    TextureType type,
    const std::string& name
  );
  void deinit();

  SDL_GPUTexture* get() const { return texture; }

  static SDL_GPUTextureFormat getGPUTextureFormat(SDL_PixelFormat format);
};
