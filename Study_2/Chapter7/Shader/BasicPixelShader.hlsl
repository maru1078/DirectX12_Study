#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1));
	float brightness = dot(-light, input.normal); // �P�ʃx�N�g���Ȃ̂œ��ς����̂܂�Cos�Ƃ̒l�ƂȂ�

	return float4(brightness, brightness, brightness, 1.0);
}