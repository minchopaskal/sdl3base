#pragma once

#include "game_state.h"
#include "config/config.h"
#include "gpu_shared/cpu_gpu_shared.h"
#include "render/gpu.h"
#include "render/renderer.h"

#include <SDL3/SDL.h>

struct AppState {
  GPUContext gpuCtx;
  Renderer renderer;
  RenderData renderData;

  GameState gameState;

  double dt = 0;
  double elapsedTime = 0;
  Uint64 lastStep = 0;
  Uint64 frameCount = 0;
  Uint64 measurementTime = 0;

  ~AppState() {
    deinit();
  }

  SDL_AppResult init(int argc, char **argv);
  void deinit();

  bool update();

  ShaderConstData getShaderConstData();
};
