#include "defines.h"
#include "shader.h"
#include "config/config.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>

#include <SDL3_shadercross/SDL_shadercross.h>

SDL_GPUShaderStage getShaderStage(
  const std::filesystem::path &in
) {
  std::string ext = in.stem().extension().string();

  if (ext == ".vert") {
    return SDL_GPU_SHADERSTAGE_VERTEX;
  } else if (ext == ".frag") {
    return SDL_GPU_SHADERSTAGE_FRAGMENT;
  }

  assert(false);
}

std::filesystem::path getTimestamp(
  const std::filesystem::path &in,
  const std::filesystem::path &outputDir
) {
  auto name = in.stem();
  name += ".timestamp";
  return outputDir / name;
}

void invalidateTimestamp(const std::filesystem::path &in) {
  using namespace std::chrono_literals;

  auto writeTime = std::filesystem::last_write_time(in);
  std::filesystem::last_write_time(in, writeTime + 1h);
}

// TODO: if we have includes we should check their timestamp too...
bool checkAndUpdateTimestamp(
  const std::filesystem::path &in,
  const std::filesystem::path &outputDir
) {
  auto timestampPath = getTimestamp(in, outputDir);
  auto currTs = std::format("{}", std::filesystem::last_write_time(in));

  if (!std::filesystem::exists(timestampPath)) {
    std::ofstream ofs(timestampPath.c_str());
    if (!ofs.is_open()) {
      assert(false);
      return false;
    }

    ofs << currTs;
    return false;
  }

  std::ifstream ifs(timestampPath.c_str());
  if (!ifs.is_open()) {
    assert(false);
    return false;
  }

  std::stringstream ss;
  ss << ifs.rdbuf();
  ifs.close();

  std::string oldTs = ss.str();
  if (oldTs != currTs) {
    std::ofstream ofs(timestampPath.c_str());
    if (!ofs.is_open()) {
      assert(false);
      return false;
    }

    ofs << currTs;
    return false;
  }

  return true;
}

std::filesystem::path getOutputName(
  const std::filesystem::path &in,
  const std::filesystem::path &outputDir,
  SDL_GPUShaderFormat format
) {
  const char *ext = nullptr;

  switch (format) {
  case SDL_GPU_SHADERFORMAT_MSL:
    ext = ".msl";
    break;
  case SDL_GPU_SHADERFORMAT_DXIL:
    ext = ".dxil";
    break;
  case SDL_GPU_SHADERFORMAT_SPIRV:
    ext = ".spirv";
    break;
  default:
    break;
  }

  auto name = in.stem();
  return (outputDir / name).concat(ext);
}

bool compileShader(
  const std::filesystem::path &in,
  const std::filesystem::path &out,
  SDL_GPUShaderStage gpuStage,
  SDL_GPUShaderFormat format
) {
  // Source is HLSL
  // if we are on windows we want to compile directly to DXIL
  // if we are on Linux or Mac we'll compile to SPIR-V
  //   - SPIR-V is directly consumed by Vulkan so we are set on Linux
  //   - for Mac we should translate SPIR-V to MSL

  auto name = in.filename();

  SDL_ShaderCross_ShaderStage stage =
    gpuStage == SDL_GPU_SHADERSTAGE_VERTEX ?
    SDL_SHADERCROSS_SHADERSTAGE_VERTEX :
    SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT;

  std::ifstream ifs(in.c_str());
  if (!ifs.is_open()) {
    printf("Failed to open shader file %s\n", in.c_str());
    return false;
  }
  std::stringstream sourceStream;
  sourceStream << ifs.rdbuf();
  std::string source = sourceStream.str();

  char hlsl[] = { '_', '_', 'H', 'L', 'S', 'L', '_', '_', '\0' };
  char vertex[] = { '_', '_', 'V', 'E', 'R', 'T', 'E', 'X', '_', '_', '\0' };
  char fragment[] = { '_', '_', 'F', 'R', 'A', 'G', 'M', 'E', 'N', 'T', '_', '_', '\0' };
  SDL_ShaderCross_HLSL_Define defines[] = {
    { hlsl, nullptr },
    { gpuStage == SDL_GPU_SHADERSTAGE_FRAGMENT ? fragment : vertex, nullptr },
    { nullptr, nullptr },
  };

  auto cwd = std::filesystem::current_path();
  auto cfgParentPath = std::filesystem::weakly_canonical(getConfig().dir);
  auto relResPath = std::filesystem::relative(cfgParentPath, cwd);

  SDL_ShaderCross_HLSL_Info hlslInfo = {
    .source = source.c_str(),
    .entrypoint = "main",
    .include_dir = relResPath.c_str(),
    .defines = defines,
    .shader_stage = stage,
#ifdef __DEBUG
    .enable_debug = true,
#else
    .enable_debug = false,
#endif
    .name = name.c_str(),
  };

  if (format == SDL_GPU_SHADERFORMAT_DXIL) {
    std::size_t size;
    void *compiled = nullptr;
    SDL_CHECK((compiled = SDL_ShaderCross_CompileDXILFromHLSL(&hlslInfo, &size)));

    std::ofstream ofs(out.c_str());
    ofs.write(reinterpret_cast<char*>(compiled), size);

    SDL_free(compiled);

    return true;
  }

  std::size_t spirvSize;
  void *compiled = nullptr;
  SDL_CHECK((compiled = SDL_ShaderCross_CompileSPIRVFromHLSL(&hlslInfo, &spirvSize)));

  if (format == SDL_GPU_SHADERFORMAT_SPIRV) {
    std::ofstream ofs(out.c_str());
    ofs.write(reinterpret_cast<char*>(compiled), spirvSize);

    SDL_free(compiled);

    return true;
  }

  assert(format == SDL_GPU_SHADERFORMAT_MSL);

  SDL_ShaderCross_SPIRV_Info spirvInfo = {
    .bytecode = reinterpret_cast<Uint8*>(compiled),
    .bytecode_size = spirvSize,
    .entrypoint = "main",
    .shader_stage = stage,
#ifdef __DEBUG
    .enable_debug = true,
#else
    .enable_debug = false,
#endif
    .name = name.c_str(),
  };

  void *compiledMSL = nullptr;
  SDL_CHECK((compiledMSL = SDL_ShaderCross_TranspileMSLFromSPIRV(&spirvInfo)));
  char *msl = reinterpret_cast<char*>(compiledMSL);

  std::ofstream ofs(out.c_str());
  ofs.write(msl, strlen(msl));

  SDL_free(msl);
  SDL_free(compiled);

  return true;
}

SDL_GPUShader* createShader(
  SDL_GPUDevice *device,
  const std::filesystem::path &in,
  const std::filesystem::path &shadersOutputDir,
  uint32_t numSamplers,
  uint32_t numBuffers
) {
  std::filesystem::path out;
  SDL_GPUShaderFormat format;
  auto supportedFormats = SDL_GetGPUShaderFormats(device);
  if (supportedFormats & SDL_GPU_SHADERFORMAT_DXIL) {
    format = SDL_GPU_SHADERFORMAT_DXIL;
  } else if (supportedFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
    format = SDL_GPU_SHADERFORMAT_SPIRV;
  } else if (supportedFormats & SDL_GPU_SHADERFORMAT_MSL) {
    format = SDL_GPU_SHADERFORMAT_MSL;
  }
  out = getOutputName(in, shadersOutputDir, format);

  SDL_GPUShaderStage type = getShaderStage(in);
  if (!std::filesystem::exists(out) || !checkAndUpdateTimestamp(in, shadersOutputDir)) {
    DEBUG_PRINT("Compiling shader %s...\n", out.filename().c_str());
    if (!compileShader(in, out, type, format)) {
      invalidateTimestamp(in);
      return nullptr;
    }
  }

  std::size_t codeSize;
  void *code = nullptr;
  SDL_CHECK_RET((code = SDL_LoadFile(out.c_str(), &codeSize)), nullptr);

  SDL_GPUShaderCreateInfo info = {
    .code_size = codeSize,
    .code = reinterpret_cast<const Uint8*>(code),
    .entrypoint = (format == SDL_GPU_SHADERFORMAT_MSL ? "main0" : "main"),
    .format = format,
    .stage = type,

    .num_samplers = numSamplers,
    .num_storage_textures = 0,
    .num_storage_buffers = numBuffers,
    .num_uniform_buffers = 1,
  };

  SDL_GPUShader *shader = nullptr;
  SDL_CHECK_RET((shader = SDL_CreateGPUShader(device, &info)), nullptr);

  SDL_free(code);

  return shader;
}
