#pragma once

#include <filesystem>

class SDL_GPUDevice;
class SDL_GPUShader;

SDL_GPUShader* createShader(
  SDL_GPUDevice *device,
  const std::filesystem::path &inputFile,
  const std::filesystem::path &shadersOutputDir,
  uint32_t numSamplers,
  uint32_t numBuffers
);
