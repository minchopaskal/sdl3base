#include "config.h"

#include "defines.h"

#include "magic_enum/magic_enum.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>

using Tokens = std::vector<std::string_view>;

Tokens tokenize(std::string_view str) {
  Tokens result;

  size_t start = 0;
  size_t i = 0;
  while (i < str.size()) {
    while (i < str.size() && str[i] != ' ' && str[i] != '\n') {
      ++i;
    }

    if (i - start > 0)
      result.push_back(str.substr(start, i - start));

    while (i < str.size() && (str[i] == ' ' || str[i] == '\n')) {
      ++i;
    }
    start = i;
  }
  if (i - start > 0)
    result.push_back(str.substr(start, i - start));

  return result;
}

#define PARSE_COMMON(sz) \
do { \
  if (toks[0] != name) return false; \
  assert(toks.size() >= (sz)); \
} while (false)

template <class T>
bool parseNumericImpl(Tokens &toks, int off, T &val) {
#if defined(__APPLE__) && defined(__clang__)
    // Apple clang does not support from_chars for float...
    char *str_end = nullptr;
    T val_ = std::strtof(toks[off].data(), &str_end);
    if (val_ == 0 && str_end == toks[off].data()) {
      return false;
    }
    if (val_ == HUGE_VALF) {
      return false;
    }

    val = val_;
    return true;
#else
  T val_;
  std::from_chars_result res;
  res = std::from_chars(
    toks[off].data(),
    toks[off].data() + toks[off].size(),
    val_
  );

  if (res.ec != std::errc()) {
    return false;
  }

  val = val_;
  return true;
#endif
}

template <class T>
bool parseNumeric(Tokens &toks, std::string_view name, T &val) {
  PARSE_COMMON(2);
  return parseNumericImpl(toks, 1, val);
}

bool parseStringImpl(Tokens &toks, int off, std::string &val) {
  val = toks[1];
  return true;
}

bool parseString(Tokens &toks, std::string_view name, std::string &val) {
  PARSE_COMMON(2);
  return parseStringImpl(toks, 1, val);
}

template <class T>
bool parseEnumImpl(Tokens &toks, int off, T &val) {
  auto val_opt = magic_enum::enum_cast<T>(toks[off]);
  if (!val_opt) {
    return false;
  }

  val = *val_opt;
  return true;
}

template <class T>
bool parseEnum(Tokens &toks, std::string_view name, T &val) {
  PARSE_COMMON(2);
  return parseEnumImpl(toks, 1, val);
}

template <int N, class T>
requires AnyOfConcept<T, int, float>
bool parseVecImpl(Tokens &toks, int off, glm::vec<N, T> &res) {
  glm::vec<N, T> res_;

  for (int i = 0; i < N; ++i) {
    if (!parseNumericImpl(toks, off + i, res_[i])) {
      return false;
    }
  }

  res = res_;
  return true;
}

template <int N, class T>
requires AnyOfConcept<T, int, float>
bool parseVec(Tokens &toks, std::string_view name, glm::vec<N, T> &res) {
  PARSE_COMMON(1 + N);
  return parseVecImpl(toks, 1, res);
}

const auto& parseVec2i = parseVec<2, int>;
const auto& parseVec3i = parseVec<3, int>;
const auto& parseVec2f = parseVec<2, float>;
const auto& parseVec3f = parseVec<3, float>;

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
    if (line.starts_with("#")) {
      continue;
    }
    auto p = tokenize(line);
    // for (auto tok : p) printf("\"%s\" ", std::string{tok}.c_str());
    // printf("\n");

    if (parseNumeric(p, "window_width", windowW)) {
      continue;
    }
    if (parseNumeric(p, "window_height", windowH)) {
      continue;
    }
    if (parseVec2i(p, "vec2i", vec2i)) {
      continue;
    }
    if (parseVec3i(p, "vec3i", vec3i)) {
      continue;
    }
    if (parseVec2f(p, "vec2f", vec2f)) {
      continue;
    }
    if (parseVec3f(p, "vec3f", vec3f)) {
      continue;
    }
    if (p[0] == "shader_input") {
      assert(p.size() >= 2);
      shadersInputDir = cfgDir / p[1];
      continue;
    }
    if (p[0] == "shader_output") {
      assert(p.size() >= 2);
      shadersOutputDir = cfgDir / p[1];
      continue;
    }

    if (parseString(p, "str", str)) {
      continue;
    }
    if (parseEnum(p, "e", e)) {
      continue;
    }
  }

  DEBUG_PRINT(
    "CFG: dir: %s str: %s e: %s(%d) vec2i: (%d %d) vec3f (%f %f %f)\n",
    dir.c_str(),
    str.c_str(),
    magic_enum::enum_name(e).data(),
    static_cast<int>(e),
    vec2i.x, vec2i.y,
    vec3f.x, vec3f.y, vec3f.z
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
