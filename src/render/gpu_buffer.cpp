#include "gpu_buffer.h"
#include "defines.h"

SDL_GPUBufferUsageFlags getUsageFlags(BufferType type) {
  switch (type) {
  case BufferType::VERTEX:
    return SDL_GPU_BUFFERUSAGE_VERTEX;
  case BufferType::INDEX:
    return SDL_GPU_BUFFERUSAGE_INDEX;
  case BufferType::STORAGE:
    return SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
  default:
    TODO();
    return 0;
  }
}

bool GPUBuffer::init(SDL_GPUDevice *device, Uint32 size, BufferType type) {
  assert(device != nullptr);
  assert(size != 0);
  assert(type != BufferType::INVALID);

  if (buffer != nullptr && type == this->type && size <= this->size) {
    return true;
  }

  deinit();

  SDL_GPUBufferCreateInfo bufInfo = {
    .usage = getUsageFlags(type),
    .size = static_cast<Uint32>(size)
  };

  SDL_CHECK(
    (buffer = SDL_CreateGPUBuffer(
      device,
      &bufInfo
    ))
  );

  this->type = type;
  this->size = size;
  this->device = device;

  return true;
}

void GPUBuffer::deinit() {
  if (buffer != nullptr) {
    SDL_ReleaseGPUBuffer(device, buffer);
  }
  device = nullptr;
  buffer = nullptr;
  size = 0;
  type = BufferType::INVALID;
}
