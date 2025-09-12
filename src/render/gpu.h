#pragma once

#include "defines.h"
#include "render/gpu_buffer.h"
#include "render/gpu_texture.h"

#include <array>
#include <optional>
#include <string_view>
#include <vector>
#include <queue>

#include <SDL3/SDL_gpu.h>
#include <SDL3_image/SDL_image.h>

using GPUFence = std::size_t;

struct Config;

template <class T>
struct UploadBuffer {
  // Data to be uploaded to the GPU.
  // MUST NOT be empty!
  const std::vector<T> &data;

  // Pointer to the GPUBuffer where to store the result.
  // MUST NOT be null!
  GPUBuffer *result;

  BufferType usage;
};

// Pass to GpuContext::upload to upload a texture
// Can be used together with multiple UploadTexture/UploadBuffer<T>
struct UploadTexture {
  // Name for debug purposes. Stored inside GPUTexture only in debug mode.
  std::string name;

  // If supplied will read from it.
  // If not file will be used and will be loaded inside.
  // User is responsible for destroying the surface!
  SDL_Surface **surface;

  // The file from which to read the texture.
  // If surface is NON-null - it will be ignored.
  std::string file;

  // Pointer to the GPUTexture where to store the result.
  // MUST NOT be null!
  GPUTexture *result;

  TextureType usage;
};

class GPUContext {
private:
  std::vector<SDL_GPUFence*> fences;
  std::vector<SDL_GPUTransferBuffer*> transferBuffers;
  std::queue<GPUFence> fenceStore;

public:
  SDL_Window *window = nullptr;
  SDL_GPUDevice *device = nullptr;

  ~GPUContext() {
    deinit();
    return;
  }

  bool init(const Config &cfg);
  void deinit();

  template <class ...Buffers>
  std::optional<GPUFence> uploadAsync(Buffers...);

  template <class ...Buffers>
  bool upload(Buffers...);

  bool wait(GPUFence fence);

private:
  GPUFence getTransferFenceHandle(SDL_GPUFence *fence, SDL_GPUTransferBuffer *transferBuffer);

  bool uploadTexture(
    const UploadTexture &tex,
    SDL_GPUCopyPass *copyPass,
    SDL_GPUTransferBuffer* transferBuffer,
    std::size_t &offset
  );

  template <class T>
  bool uploadBuffer(
    UploadBuffer<T> buf,
    SDL_GPUCopyPass *copyPass,
    SDL_GPUTransferBuffer* transferBuffer,
    std::size_t &offset
  );
};

template <class ...Buffers>
std::optional<GPUFence> GPUContext::uploadAsync(Buffers ...buffers) {
  auto ensureSurface = [] (auto &buf) -> bool {
    if constexpr (!std::is_same_v<std::remove_cvref_t<decltype(buf)>, UploadTexture>) {
      return true;
    } else {
      auto &tex = buf;
      assert(tex.surface != nullptr);

      if (*tex.surface == nullptr) {
        SDL_CHECK((*tex.surface = IMG_Load(tex.file.c_str())));
      }

      auto &surface = *tex.surface;
      if (surface->format != SDL_PIXELFORMAT_RGBA32) {
        auto oldSurface = surface;
        surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(oldSurface);
        SDL_CHECK(surface != nullptr);
      }

      return true;
    }
  };

  // If buffer is texture we want to make sure its image is loaded into an SDL_Surface
  bool res = true;
  ((res = res && ensureSurface(buffers)), ...);
  if (!res) {
    return std::nullopt;
  }

  auto getBufSize = [](auto &buf) {
    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(buf)>, UploadTexture>) {
      return (*buf.surface)->pitch * (*buf.surface)->h;
    } else {
      return buf.data.size() * sizeof(buf.data[0]);
    }
  };

  std::size_t transferBufSz = (
    0 +
    ... +
    getBufSize(buffers)
  );
  SDL_GPUTransferBufferCreateInfo tbInfo = {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = static_cast<Uint32>(transferBufSz)
  };
  SDL_GPUTransferBuffer* transferBuffer = nullptr;
  SDL_CHECK_RET(
    (transferBuffer = SDL_CreateGPUTransferBuffer(
      device,
      &tbInfo
    )),
    std::nullopt
  );

  void *transferData = nullptr;
  SDL_CHECK_RET(
    (transferData = (void*)SDL_MapGPUTransferBuffer(
      device,
      transferBuffer,
      false
    )),
    std::nullopt
  );

  auto setTransferData = [&getBufSize] (
    const auto &buf,
    void *transferData,
    std::size_t &offset
  ) {
    uint8_t *data = reinterpret_cast<uint8_t*>(transferData) + offset;

    std::size_t sz = getBufSize(buf);
    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(buf)>, UploadTexture>) {
      memcpy(data, (*buf.surface)->pixels, sz);
    } else {
      memcpy(data, buf.data.data(), sz);
    }

    offset += sz;
  };

  std::size_t offset = 0;
  (setTransferData(buffers, transferData, offset), ...);

  SDL_UnmapGPUTransferBuffer(device, transferBuffer);

  SDL_GPUCommandBuffer *cmdBuf = nullptr;
  SDL_CHECK_RET(
    (cmdBuf = SDL_AcquireGPUCommandBuffer(device)),
    std::nullopt
  );

	SDL_GPUCopyPass* copyPass = nullptr;
  SDL_CHECK_RET((copyPass = SDL_BeginGPUCopyPass(cmdBuf)), std::nullopt);

  auto uploadBuf = [this] (
    const auto &buf,
    SDL_GPUCopyPass *copyPass,
    SDL_GPUTransferBuffer *transferBuffer,
    std::size_t &offset
  ) -> bool {
    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(buf)>, UploadTexture>) {
      return uploadTexture(
        buf,
        copyPass,
        transferBuffer,
        offset
      );
    } else {
      return uploadBuffer<std::remove_cvref_t<decltype(buf.data[0])>>(
        buf,
        copyPass,
        transferBuffer,
        offset
      );
    }
  };

  offset = 0;
  res = true;
  (
    (res = res && uploadBuf(
      buffers,
      copyPass,
      transferBuffer,
      offset
    )),
    ...
  );
  if (!res) {
    return std::nullopt;
  }

  SDL_EndGPUCopyPass(copyPass);

  SDL_GPUFence *fencePtr = nullptr;
  SDL_CHECK_RET(
    (fencePtr = SDL_SubmitGPUCommandBufferAndAcquireFence(cmdBuf)),
    std::nullopt
  );

  assert(fencePtr != nullptr);
  GPUFence handle = getTransferFenceHandle(fencePtr, transferBuffer);

  return handle;
}

template <class ...Buffers>
bool GPUContext::upload(Buffers ...textures) {
  if (auto opt = uploadAsync(textures...); opt.has_value()) {
    return wait(*opt);
  }

  return false;
}

template <class T>
bool GPUContext::uploadBuffer(
  UploadBuffer<T> buf,
  SDL_GPUCopyPass *copyPass,
  SDL_GPUTransferBuffer* transferBuffer,
  std::size_t &offset
) {
  assert(buf.result != nullptr);

  auto bufferSz = static_cast<Uint32>(buf.data.size() * sizeof(std::remove_cvref_t<T>));
  buf.result->init(device, bufferSz, buf.usage);

  SDL_GPUTransferBufferLocation tbLocInfo = {
    .transfer_buffer = transferBuffer,
    .offset = static_cast<Uint32>(offset),
  };
  SDL_GPUBufferRegion bufReg = {
    .buffer = buf.result->get(),
    .offset = 0,
    .size = bufferSz
  };
  SDL_UploadToGPUBuffer(
    copyPass,
    &tbLocInfo,
    &bufReg,
    false
  );

  offset += bufferSz;
  return true;
}
