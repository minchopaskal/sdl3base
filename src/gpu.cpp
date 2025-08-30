#include "gpu.h"
#include "config/config.h"

#include <SDL3/SDL_surface.h>

bool GPUContext::init(const Config &cfg) {
  deinit();

  std::string shadersInput = cfg.shadersInputDir;
  std::string shadersOutput = cfg.shadersOutputDir;

  auto shaderFormats =
    SDL_GPU_SHADERFORMAT_SPIRV |
    SDL_GPU_SHADERFORMAT_DXIL |
    SDL_GPU_SHADERFORMAT_MSL ;

  SDL_CHECK((device = SDL_CreateGPUDevice(
    shaderFormats,
#ifdef __DEBUG
    true,
#else
    false,
#endif
    nullptr
  )));

  SDL_CHECK((window = SDL_CreateWindow(
    PROJECT_NAME,
    cfg.windowW,
    cfg.windowH,
    0
  )));

  SDL_CHECK(SDL_ClaimWindowForGPUDevice(device, window));

  return true;
}

void GPUContext::deinit() {
  for (int i = 0; i < fences.size(); ++i) {
    wait(i);
  }

  if (device && window) {
    SDL_ReleaseWindowFromGPUDevice(device, window);
  }

  if (window) {
    SDL_DestroyWindow(window);
    window = nullptr;
  }

  if (device) {
    SDL_DestroyGPUDevice(device);
    device = nullptr;
  }
}

bool GPUContext::wait(GPUFence idx) {
  // TODO: make thread-safe if needed
  assert(idx < fences.size());

  auto fence = fences[idx];
  if (fence == nullptr) {
    return true;
  }

  SDL_CHECK_RET(SDL_WaitForGPUFences(device, true, &fence, 1), false);

  SDL_ReleaseGPUFence(device, fence);
  fences[idx] = nullptr;
  fenceStore.push(idx);

  assert(transferBuffers[idx] != nullptr);
  SDL_ReleaseGPUTransferBuffer(device, transferBuffers[idx]);
  transferBuffers[idx] = nullptr;

  return true;
}

GPUFence GPUContext::getTransferFenceHandle(SDL_GPUFence *fence, SDL_GPUTransferBuffer *transferBuffer) {
  // TODO: Make thread-safe if needed
  if (fenceStore.empty()) {
    fences.push_back(fence);
    transferBuffers.push_back(transferBuffer);
    return fences.size() - 1;
  }

  auto handle = fenceStore.front();
  fenceStore.pop();
  fences[handle] = fence;

  // reuse the handle for transferBuffers for ease of use
  transferBuffers.resize(std::max(transferBuffers.size(), handle + 1));
  transferBuffers[handle] = transferBuffer;

  return handle;
}

bool GPUContext::uploadTexture(
  const UploadTexture &tex,
  SDL_GPUCopyPass *copyPass,
  SDL_GPUTransferBuffer* transferBuffer,
  std::size_t &offset
) {
  assert(tex.result != nullptr);
  assert(tex.surface != nullptr);

  auto surface = *tex.surface;
  tex.result->init(
    device,
    surface->w,
    surface->h,
    surface->format,
    tex.usage,
    tex.name
  );

  SDL_GPUTextureTransferInfo tbLocInfo = {
    .transfer_buffer = transferBuffer,
    .offset = static_cast<Uint32>(offset),
    .pixels_per_row = static_cast<Uint32>(surface->w),
    .rows_per_layer = static_cast<Uint32>(surface->h)
  };
  SDL_GPUTextureRegion bufReg = {
    .texture = tex.result->get(),
    .w = static_cast<Uint32>(surface->w),
    .h = static_cast<Uint32>(surface->h),
    .d = 1
  };
  SDL_UploadToGPUTexture(
    copyPass,
    &tbLocInfo,
    &bufReg,
    false
  );

  offset += surface->pitch * surface->h;
  return true;
}
