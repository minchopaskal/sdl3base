#include "app_state.h"

#include "defines.h"
#include "gpu_shared/cpu_gpu_shared.h"

SDL_AppResult AppState::init(int argc, char **argv) {
  deinit();

  if (argc < 2) {
    printf("Invalid number of cmd args!\n");
    return SDL_APP_FAILURE;
  }

  std::string cfgPath = argv[1];
  if (!getConfig().parse(cfgPath)) {
    printf("Failed to initialize config!\n");
    return SDL_APP_FAILURE;
  }

  SDL_CHECK_APP(SDL_SetAppMetadata(PROJECT_NAME, "1.0.0", PROJECT_NAME));

  SDL_CHECK_APP(SDL_Init(SDL_INIT_VIDEO));

  if (!gpuCtx.init(getConfig())) {
    return SDL_APP_FAILURE;
  }

  if (!renderer.init(&gpuCtx)) {
    return SDL_APP_FAILURE;
  }

  gameState.generate(getConfig());

  lastStep = SDL_GetTicks();

  return SDL_APP_CONTINUE;
}

void AppState::deinit() {
  DEBUG_PRINT("DEINIT: %s\n", "AppState");

  renderData.deinit();

  renderer.deinit();

  gpuCtx.deinit();

  SDL_Quit();
}

bool AppState::update() {
  gameState.update(dt);

  return true;
}

ShaderConstData AppState::getShaderConstData() {
  glm::dvec2 windowSz{ getConfig().windowW, getConfig().windowH };

  return ShaderConstData {
    .windowSize = windowSz,
    .time = static_cast<float>(elapsedTime),
    .delta = static_cast<float>(dt),

#ifdef PRJ_DEBUG
    .debug = true,
#else
    .debug = false,
#endif // PRJ_DEBUG
  };
}

