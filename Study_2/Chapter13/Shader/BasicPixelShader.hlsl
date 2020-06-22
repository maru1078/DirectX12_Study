#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	// �e�`��
	if (input.instNo == 1)
	{
		return float4(0, 0, 0, 0);
	}

	float3 light = normalize(float3(1, -1, 1));
	float brightness = dot(-light, input.normal); // �P�ʃx�N�g���Ȃ̂œ��ς����̂܂�Cos�Ƃ̒l�ƂȂ�
	float2 normalUV = (input.normal.xy + float2(1, -1)) * float2(0.5, -0.5);
	float2 sphereMapUV = (input.vnormal.xy + float2(1, -1)) * float2(0.5, -0.5);
	float4 texColor = tex.Sample(smp, input.uv); // �e�N�X�`���J���[

	// �f�B�t���[�Y�v�Z
	float diffuseB = saturate(dot(-light, input.normal));
	float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	// ���̔��˃x�N�g��
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	float3 posFromLightVP = input.tpos.xyz / input.tpos.w;
	float2 shadowUV = (posFromLightVP + float2(1, -1)) * float2(0.5, -0.5);
	float depthFromLight = lightDepthTex.SampleCmp(
		shadowSmp, // ��r�T���v���[
		shadowUV, // uv�l
		posFromLightVP.z - 0.005); // ��r�Ώےl
	float shadowWeight = lerp(0.5, 1.0, depthFromLight);
	//if (depthFromLight < posFromLightVP.z - 0.001)
	//{
	//	shadowWeight = 0.5;
	//}

	toonDif *= shadowWeight;

	return max(saturate(
		toonDif // �f�B�t���[�Y�F
		* diffuse
		* texColor
		* sph.Sample(smp, sphereMapUV))  
		+ saturate(spa.Sample(smp, sphereMapUV)
		+ float4(specularB * specular.rgb, 1)) // �X�y�L����
		, float4(ambient * texColor, 1)); // �A���r�G���g
}