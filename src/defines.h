#pragma once

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <source_location>

#define PROJECT_NAME "Prj"

#define SDL_CHECK_RET(expr, ret) \
do { \
  if (!(expr)) { \
    auto loc = std::source_location::current(); \
    auto p = std::filesystem::path(loc.file_name()); \
    printf("Expression %s failed at %s:%d with error: %s\n", \
      #expr, \
      p.filename().c_str(), \
      loc.line(), \
      SDL_GetError() \
    ); \
    return (ret); \
  } \
} while (false)

#define SDL_CHECK(expr) SDL_CHECK_RET(expr, false)

#define SDL_CHECK_APP(expr) SDL_CHECK_RET(expr, SDL_APP_FAILURE)

#ifdef SHOW_MEASURE

#define SDL_MEASURE(expr, what) \
do { \
  Uint64 start = SDL_GetTicks(); \
  (expr); \
  Uint64 end = SDL_GetTicks(); \
  printf("%s: %llums\n", what, end - start); \
} while (false)

#define SDL_MEASURE_RET(expr, what) \
  [&] { \
    Uint64 start = SDL_GetTicks(); \
    auto res = expr; \
    Uint64 end = SDL_GetTicks(); \
    printf("%s: %llums\n", what, end - start); \
    return res; \
  } ()

#else

#define SDL_MEASURE(expr, what) expr

#define SDL_MEASURE_RET(expr, what) expr

#endif // SHOW_MEASURE

#ifdef __DEBUG

#define DEBUG_PRINT(format, ...) \
do { \
  printf(format, __VA_ARGS__); \
} \
while (false)

#define TODO() \
do { \
  assert(false); \
  return {}; \
} while (false)

#define UNREACHABLE() TODO()

#else

#define DEBUG_PRINT(...) (void)0
#define UNREACHABLE() (void)0
#define TODO() do {\
  return {}; \
} while (false)

#endif // __DEBUG
