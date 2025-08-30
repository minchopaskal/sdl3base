#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>

#include "app_state.h"
#include "defines.h"

SDL_AppResult SDL_AppInit(void **as, int argc, char *argv[]) {
  DEBUG_PRINT("CWD: %s\n", std::filesystem::current_path().c_str());

  initConfig();

  AppState *appState = new AppState;
  *as = appState;

  return appState->init(argc, argv);
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  AppState *as = (AppState*)appstate;

  as->update();

  SDL_CHECK_APP(as->renderer.draw(as->getShaderConstData(), as->renderData));

  auto last = as->lastStep;
  as->lastStep = SDL_GetTicks();

  ++as->frameCount;
  as->measurementTime += as->lastStep - last;
  as->dt = (as->lastStep - last) / 1000.;
  as->elapsedTime += as->dt;

  if (as->measurementTime >= 1000) {
    printf("FRAME: %f FPS\n", double(as->frameCount * 1000) / as->measurementTime);
    as->measurementTime = 0;
    as->frameCount = 0;
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  auto as = (AppState*)appstate;

  switch (event->type) {
  case SDL_EVENT_QUIT:
    DEBUG_PRINT("SDL_EVENT_QUIT %s\n", ""); // TODO: debug_print without args
    return SDL_APP_SUCCESS;

  default:
    break;
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  if (appstate != nullptr) {
    AppState *as = reinterpret_cast<AppState*>(appstate);
    delete as;
  }

  deinitConfig();
}

