#include "gpu_shared/cpu_gpu_shared.h"

ConstantBuffer<ShaderConstData> renderData : register(b0, space1);

struct VSOutput {
	float2 uv : TEXCOORD0;
	float4 position : SV_Position;
};

VSOutput main(in uint vertID : SV_VertexID) {
	VSOutput result;

	result.uv = float2(uint2(vertID, vertID << 1) & 2);
	result.position = float4(lerp(float2(-1, 1), float2(1, -1), result.uv), 0.f, 1.f);

	return result;
}
