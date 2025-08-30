#pragma once

#ifndef __HLSL__

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#define uint uint32_t
#define uint2 glm::uvec2
#define float3 glm::vec3
#define float2 glm::vec2

#endif

struct ShaderConstData {
  float2 windowSize;
  float time;
  float delta;

  uint debug;
};

#ifndef __HLSL__

#undef uint
#undef uint2
#undef float2
#undef float3

#endif

