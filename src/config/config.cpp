#include "config.h"

#include "defines.h"

#include "magic_enum/magic_enum.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>

using TokenPair = std::pair<std::string_view, std::string_view>;

TokenPair tokenize(std::string_view str) {
  TokenPair result;

  size_t i = 0;
  while (i < str.size() && str[i] != ' ') {
    ++i;
  }
  result.first = str.substr(0, i);

  while (i < str.size() && str[i] == ' ') {
    ++i;
  }

  size_t begin = i;
  while (i < str.size() && !std::isspace(str[i])) {
    ++i;
  }
  result.second = str.substr(begin, i - begin);

  return result;
}

template <std::integral I>
bool parseIntegral(TokenPair toks, std::string_view name, I &val) {
  if (toks.first != name) {
    return false;
  }

  auto res = std::from_chars(
    toks.second.data(),
    toks.second.data() + toks.second.size(),
    val
  );

  if (res.ec != std::errc()) {
    return false;
  }

  return true;
}

bool parseString(TokenPair toks, std::string_view name, std::string &val) {
  if (toks.first != name) {
    return false;
  }

  val = toks.second;
  return true;
}

template <class T>
bool parseEnum(TokenPair toks, std::string_view name, T &val) {
  if (toks.first != name) {
    return false;
  }

  auto val_opt = magic_enum::enum_cast<T>(toks.second);
  if (!val_opt) {
    return false;
  }

  val = *val_opt;
  return true;
}

bool Config::parse(std::string_view path) {
  std::filesystem::path cfgPath(path);

  std::ifstream ifs(cfgPath.c_str());
  if (!ifs.is_open()) {
    fprintf(stderr, "Failed to open config %s\n", path.data());
    return false;
  }

  auto cfgDir = cfgPath.parent_path();
  dir = cfgDir;

  std::string line;
  while (std::getline(ifs, line)) {
    if (line.empty()) {
      continue;
    }
    auto p = tokenize(line);

    if (parseIntegral(p, "window_width", windowW)) {
      continue;
    }
    if (parseIntegral(p, "window_height", windowH)) {
      continue;
    }
    if (p.first == "shader_input") {
      shadersInputDir = cfgDir / p.second;
      continue;
    }
    if (p.first == "shader_output") {
      shadersOutputDir = cfgDir / p.second;
      continue;
    }

    // examples
    if (parseString(p, "str", str)) {
      continue;
    }
    if (parseEnum(p, "e", e)) {
      continue;
    }
  }

  DEBUG_PRINT(
    "CFG: dir: %s str: %s e: %s(%d)\n",
    dir.c_str(),
    str.c_str(),
    magic_enum::enum_name(e).data(),
    static_cast<int>(e)
  );

  return true;
}

Config *cfg = nullptr;

void initConfig() {
  deinitConfig();
  cfg = new Config;
}

Config &getConfig() {
  assert(cfg != nullptr);
  return *cfg;
}

void deinitConfig() {
  if (cfg == nullptr) return;
  delete cfg;
}
