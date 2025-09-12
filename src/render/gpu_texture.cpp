#include "gpu_texture.h"
#include "defines.h"

SDL_GPUTextureUsageFlags getTextureUsage(TextureType type) {
  switch (type) {
  case TextureType::SAMPLER:
    return SDL_GPU_TEXTUREUSAGE_SAMPLER;
  case TextureType::TARGET:
    // If we are going to use the texture as target most probably we will sample
    // it on some later pass.
    return SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
  case TextureType::DEPTH:
    return SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
  case TextureType::STORAGE:
    return SDL_GPU_TEXTUREUSAGE_GRAPHICS_STORAGE_READ;
  }

  assert(false);
  return (SDL_GPUTextureUsageFlags)0;

}

SDL_GPUTextureCreateInfo getTextureInfo(
  uint32_t w,
  uint32_t h,
  SDL_PixelFormat format,
  TextureType type
) {
  SDL_GPUTextureCreateInfo res = {
    .type = SDL_GPU_TEXTURETYPE_2D,
    .format = GPUTexture::getGPUTextureFormat(format),
    .usage = getTextureUsage(type),
    .width = w,
    .height = h,
    .layer_count_or_depth = 1,
    .num_levels = 1,
    .sample_count = SDL_GPU_SAMPLECOUNT_1
  };

  return res;
}

bool GPUTexture::init(
  SDL_GPUDevice *device,
  uint32_t w,
  uint32_t h,
  SDL_PixelFormat format,
  TextureType type,
  const std::string &name
) {
  deinit();

  SDL_GPUTextureCreateInfo texInfo = getTextureInfo(w, h, format, type);

  DEBUG_PRINT("Creating texture... %s\n", name.c_str());
  SDL_CHECK((
    texture = SDL_CreateGPUTexture(
      device,
      &texInfo
    )
  ));

  sz = {w, h};

#ifdef __DEBUG
  this->name = name;
#endif
  return true;
}

void GPUTexture::deinit() {
  if (texture != nullptr) {
    DEBUG_PRINT("Releasing texture... %s\n", name.c_str());
    SDL_ReleaseGPUTexture(device, texture);
  }

  texture = nullptr;
  device = nullptr;
}

SDL_GPUTextureFormat GPUTexture::getGPUTextureFormat(SDL_PixelFormat format) {
  if (format == SDL_PIXELFORMAT_RGBA32) {
    return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
  }

  TODO();
}
