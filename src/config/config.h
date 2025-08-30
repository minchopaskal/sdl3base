#pragma once

#include <filesystem>
#include <optional>
#include <string>

enum class EnumType {
  FST,
  SND,
};

struct Config {
  std::filesystem::path dir;
  std::string shadersInputDir;
  std::string shadersOutputDir;
  uint windowW, windowH;

  std::string str;
  EnumType e;

  bool parse(std::string_view path);
};

Config &getConfig();
void initConfig();
void deinitConfig();
