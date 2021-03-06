#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1));
	float brightness = dot(-light, input.normal); // 単位ベクトルなので内積がそのままCosθの値となる

	return float4(brightness, brightness, brightness, 1.0);
}