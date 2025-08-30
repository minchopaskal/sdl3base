#pragma once

struct Config;

#include <glm/vec2.hpp>
#include "gpu_shared/cpu_gpu_shared.h"

struct GameState {
  void generate(const Config& cfg);
  void update(double deltaTime);
};
