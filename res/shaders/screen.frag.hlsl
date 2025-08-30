#include "gpu_shared/cpu_gpu_shared.h"

ConstantBuffer<ShaderConstData> renderData : register(b0, space3);

struct PSInput {
  float2 uv : TEXCOORD0;
};

float4 main(PSInput IN) : SV_TARGET {
  return float4(IN.uv, 0.f, 1.f);
}
