#pragma once

#include "gpu.h"
#include "gpu_shared/cpu_gpu_shared.h"

struct RenderData {
  void init() {}
  void deinit() {}
};

class Renderer {
private:
  // TODO: do we need to decouple pipeline from render pass?
  struct RenderPass {
    GPUTexture target;

    SDL_GPUDevice *device = nullptr;
    SDL_GPUGraphicsPipeline *pipeline = nullptr;

    SDL_GPUCommandBuffer *cmdBuf = nullptr;
    SDL_GPURenderPass *renderPass = nullptr;
    bool isSwapchainTarget = false;

    bool init(
      SDL_GPUDevice *device,
      SDL_Window *window,
      int targetW,
      int targetH,
      SDL_PixelFormat targetFormat,
      std::string_view shaderName,
      SDL_GPUVertexInputState inputLayout,
      uint32_t numVertexStorageBuffers,
      uint32_t numFragmentStorageBuffers,
      uint32_t numTextures,
      bool swapchainTarget
    );
    void deinit();

    bool begin(
      SDL_GPUCommandBuffer *cmdBuf,
      SDL_GPUTexture *swapchain = nullptr
    );

    void bind(
      const std::vector<SDL_GPUTexture*> &textures,
      const std::vector<SDL_GPUBuffer*> &storageBuffers,
      const std::vector<SDL_GPUBuffer*> &vertexBuffers,
      SDL_GPUBuffer *indexBuffer,
      SDL_GPUTexture *depthBuffer,
      SDL_GPUSampler *sampler
    );

    void exec(uint32_t numIndices, uint32_t numInstances);

    void end();
  };


private:
  static constexpr int FRAMES_IN_FLIGHT = 2;

  RenderPass renderPass;

  GPUContext *gpu;

  GPUBuffer screenTriIndexBuffer;

  SDL_GPUSampler *pointSampler = nullptr;

  int frameCycle = 0;
  SDL_GPUFence *fences[FRAMES_IN_FLIGHT] = { nullptr, nullptr };

public:
  bool init(GPUContext *gpu);
  void deinit();

  SDL_AppResult draw(
    const ShaderConstData &shaderConstData,
    const RenderData &renderData
  );
};
