#include "gpu_shared/cpu_gpu_shared.h"

ConstantBuffer<ShaderConstData> renderData : register(b0, space3);

Texture2D<float4> render : register(t0, space2);
SamplerState pointSampler : register(s0, space2);

struct PSInput {
  float2 uv : TEXCOORD0;
};

float4 main(PSInput IN) : SV_TARGET {
  // Swizzle the colors
  return render.Sample(pointSampler, IN.uv).bgra;
}
