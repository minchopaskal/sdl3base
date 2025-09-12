#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

enum class EnumType {
  FST,
  SND,
};

struct Config {
  std::filesystem::path dir;
  std::string shadersInputDir;
  std::string shadersOutputDir;
  uint windowW, windowH;

  glm::ivec2 vec2i;
  glm::ivec3 vec3i;
  glm::vec2 vec2f;
  glm::vec3 vec3f;
  std::string str;
  EnumType e;

  bool parse(std::string_view path);
};

Config &getConfig();
void initConfig();
void deinitConfig();
