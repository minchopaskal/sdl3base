#include "renderer.h"
#include "shader.h"
#include "config/config.h"

#include "SDL3/SDL_stdinc.h"

#include "game_state.h"

void RenderData::init(GPUContext &gpuCtx, GameState &state) {
  update(state);
}

void RenderData::update(GameState &state) {
}

void RenderData::deinit() {
}

bool Renderer::RenderPass::init(
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
) {
  this->device = device;
  isSwapchainTarget = swapchainTarget;

  auto shadersInputDir = std::filesystem::path(getConfig().shadersInputDir);
  auto shadersOutputDir = std::filesystem::path(getConfig().shadersOutputDir);

  SDL_GPUShader *vertex = createShader(
    device,
    (shadersInputDir / shaderName).concat(".vert.hlsl"),
    shadersOutputDir,
    0, /* TODO: no vertex samplers for now... */
    numVertexStorageBuffers
  );
  if (!vertex) {
    deinit();
    return false;
  }

  SDL_GPUShader *fragment = createShader(
    device,
    (shadersInputDir / shaderName).concat(".frag.hlsl"),
    shadersOutputDir,
    numTextures,
    numFragmentStorageBuffers
  );
  if (!fragment) {
    deinit();
    return false;
  }

  if (!isSwapchainTarget) {
    target.init(device, targetW, targetH, targetFormat, TextureType::TARGET, "render_target");
  }

  SDL_GPUColorTargetDescription targetDesc = {
    .format = isSwapchainTarget ?
      SDL_GetGPUSwapchainTextureFormat(device, window) :
      GPUTexture::getGPUTextureFormat(targetFormat),
  };

  SDL_GPUGraphicsPipelineTargetInfo targetInfo = {
    .color_target_descriptions = &targetDesc,
    .num_color_targets = 1,
    .has_depth_stencil_target = false,
  };

  SDL_GPUGraphicsPipelineCreateInfo info = {
    .vertex_shader = vertex,
    .fragment_shader = fragment,
    .vertex_input_state = inputLayout,
    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    .target_info = targetInfo,
  };
  SDL_CHECK_RET((pipeline = SDL_CreateGPUGraphicsPipeline(device, &info)), false);

  SDL_ReleaseGPUShader(device, vertex);
  SDL_ReleaseGPUShader(device, fragment);

  return true;
}

void Renderer::RenderPass::deinit() {
  if (pipeline != nullptr) {
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
  }

  target.deinit();

  *this = {};
}

bool Renderer::RenderPass::begin(SDL_GPUCommandBuffer *cmdBuf, SDL_GPUTexture *swapchain) {
  assert(renderPass == nullptr);

  SDL_GPUColorTargetInfo colorTargetInfo = {
    .texture = isSwapchainTarget ? swapchain : target.get(),
    .load_op = SDL_GPU_LOADOP_LOAD,
    .store_op = SDL_GPU_STOREOP_STORE,
  };

  SDL_CHECK((renderPass = SDL_BeginGPURenderPass(
    cmdBuf,
    &colorTargetInfo,
    1,
    nullptr // TODO: add depth support
  )));

  SDL_BindGPUGraphicsPipeline(
    renderPass,
    pipeline
  );

  return true;
}

void Renderer::RenderPass::bind(
  const std::vector<SDL_GPUTexture*> &textures,
  const std::vector<SDL_GPUBuffer*> &storageBuffers,
  const std::vector<SDL_GPUBuffer*> &vertexBuffers,
  SDL_GPUBuffer *indexBuffer,
  SDL_GPUTexture *depthBuffer,
  SDL_GPUSampler *sampler
) {
  assert(renderPass != nullptr);

  // Bind vertex buffers
  std::vector<SDL_GPUBufferBinding> vertexBindings;
  for (SDL_GPUBuffer *buffer : vertexBuffers) {
    vertexBindings.push_back(SDL_GPUBufferBinding {
      .buffer = buffer,
      .offset = 0
    });
  }
  if (!vertexBuffers.empty()) {
    SDL_BindGPUVertexBuffers(
      renderPass,
      0,
      vertexBindings.data(),
      static_cast<Uint32>(vertexBuffers.size())
    );
  }

  // Bind index buffer
  assert(indexBuffer != nullptr);
  SDL_GPUBufferBinding idxBufBind = {
    .buffer = indexBuffer,
    .offset = 0
  };
  SDL_BindGPUIndexBuffer(
    renderPass,
    &idxBufBind,
    SDL_GPU_INDEXELEMENTSIZE_16BIT
  );

  // Bind storage buffers
  if (!storageBuffers.empty()) {
    SDL_BindGPUFragmentStorageBuffers(
      renderPass,
      0,
      storageBuffers.data(),
      static_cast<Uint32>(storageBuffers.size())
    );
  }

  // Bind samplers
  std::vector<SDL_GPUTextureSamplerBinding> samplerBindings;
  for (SDL_GPUTexture *texture : textures) {
    samplerBindings.push_back(SDL_GPUTextureSamplerBinding {
      .texture = texture,
      .sampler = sampler
    });
  }
  if (!samplerBindings.empty()) {
    SDL_BindGPUFragmentSamplers(
      renderPass,
      0,
      samplerBindings.data(),
      static_cast<Uint32>(samplerBindings.size())
    );
  }
}

void Renderer::RenderPass::exec(uint32_t numIndices, uint32_t numInstances) {
  assert(renderPass != nullptr);

  SDL_DrawGPUIndexedPrimitives(renderPass, numIndices, numInstances, 0, 0, 0);
}

void Renderer::RenderPass::end() {
  if (renderPass != nullptr) {
    SDL_EndGPURenderPass(renderPass);
    renderPass = nullptr;
  }
}

bool Renderer::init(GPUContext *gpu) {
  assert(gpu != nullptr);

  this->gpu = gpu;

  if (!renderPass.init(
    gpu->device,
    gpu->window,
    getConfig().windowW,
    getConfig().windowH,
    SDL_PIXELFORMAT_RGBA32,
    "screen",
    {},
    0,
    0,
    0,
    false
  )) {
    return false;
  }

  if (!postprocessPass.init(
    gpu->device,
    gpu->window,
    getConfig().windowW,
    getConfig().windowH,
    SDL_PIXELFORMAT_RGBA32,
    "post",
    {},
    0,
    0,
    1,
    false
  )) {
    return false;
  }

  if (!postprocessPass2.init(
    gpu->device,
    gpu->window,
    0,
    0,
    SDL_PIXELFORMAT_UNKNOWN,
    "post2",
    {},
    0,
    0,
    1,
    true
  )) {
    return false;
  }

  std::vector<Uint16> indexDataScreenTri = {0, 1, 2};
  if (!gpu->upload(
    UploadBuffer<Uint16>{ indexDataScreenTri, &screenTriIndexBuffer, BufferType::INDEX }
  )) {
    return false;
  }

  SDL_GPUSamplerCreateInfo samplerInfo = {
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	};
  SDL_CHECK((pointSampler = SDL_CreateGPUSampler(gpu->device, &samplerInfo)));

  return true;
}

void Renderer::deinit() {
  for (int i = 0; i < FRAMES_IN_FLIGHT; ++i) {
    if (fences[i] != nullptr) {
      SDL_WaitForGPUFences(gpu->device, true, &fences[i], 1);
      SDL_ReleaseGPUFence(gpu->device, fences[i]);
      fences[i] = nullptr;
    }
  }

  if (pointSampler != nullptr) {
    SDL_ReleaseGPUSampler(gpu->device, pointSampler);
    pointSampler = nullptr;
  }

  screenTriIndexBuffer.deinit();
  renderPass.deinit();
  postprocessPass.deinit();

  gpu = nullptr;
}

SDL_AppResult Renderer::draw(
  const ShaderConstData &shaderConstData,
  const RenderData &renderData
) {
  frameCycle = (frameCycle + 1) % FRAMES_IN_FLIGHT;

  if (fences[frameCycle] != nullptr) {
    SDL_WaitForGPUFences(gpu->device, true, &fences[frameCycle], 1);
    SDL_ReleaseGPUFence(gpu->device, fences[frameCycle]);
    fences[frameCycle] = nullptr;
  }

  SDL_GPUCommandBuffer *cmdBuf;
  SDL_CHECK_APP((cmdBuf = SDL_AcquireGPUCommandBuffer(gpu->device)));

  SDL_GPUTexture *swapchain = nullptr;
  SDL_CHECK_APP(
    SDL_AcquireGPUSwapchainTexture(
      cmdBuf,
      gpu->window,
      &swapchain,
      nullptr,
      nullptr
    )
  );

  if (swapchain == nullptr) {
    return SDL_APP_CONTINUE;
  }

  SDL_PushGPUVertexUniformData(
    cmdBuf,
    0,
    &shaderConstData,
    sizeof(ShaderConstData)
  );
  SDL_PushGPUFragmentUniformData(
    cmdBuf,
    0,
    &shaderConstData,
    sizeof(ShaderConstData)
  );

  renderPass.begin(cmdBuf, swapchain);
  renderPass.bind(
    {},
    {},
    {},
    screenTriIndexBuffer.get(),
    nullptr,
    pointSampler
  );
  renderPass.exec(3, 1);
  renderPass.end();

  postprocessPass.begin(cmdBuf, swapchain);
  postprocessPass.bind(
    {renderPass.target.get()},
    {},
    {},
    screenTriIndexBuffer.get(),
    nullptr,
    pointSampler
  );
  postprocessPass.exec(3, 1);
  postprocessPass.end();

  postprocessPass2.begin(cmdBuf, swapchain);
  postprocessPass2.bind(
    {postprocessPass.target.get()},
    {},
    {},
    screenTriIndexBuffer.get(),
    nullptr,
    pointSampler
  );
  postprocessPass2.exec(3, 1);
  postprocessPass2.end();



  SDL_CHECK_APP((
    fences[frameCycle] = SDL_SubmitGPUCommandBufferAndAcquireFence(cmdBuf)
  ));

  return SDL_APP_SUCCESS;
}
