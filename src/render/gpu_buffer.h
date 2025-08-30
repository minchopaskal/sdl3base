#pragma once

#include <SDL3/SDL_gpu.h>

enum class BufferType {
  INVALID,

  INDEX,
  VERTEX,
  STORAGE,
  COMPUTE_READ,
  COMPUTE_WRITE,
  COMPUTE_RW
};

// TODO: think of linking these buffers to the device
// so that when device is destroyed it releases all its buffers
// if not already released, same for GPUTexture
struct GPUBuffer {
private:
  SDL_GPUDevice *device = nullptr;
  SDL_GPUBuffer *buffer = nullptr;
  Uint32 size = 0;
  BufferType type = BufferType::INVALID;

public:
  bool init(SDL_GPUDevice *device, Uint32 size, BufferType type);
  void deinit();

  SDL_GPUBuffer* get() const { return buffer; }
  SDL_GPUBuffer* const* getPtr() const { return &buffer; }
};
