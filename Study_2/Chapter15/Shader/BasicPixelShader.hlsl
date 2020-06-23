#include "BasicShaderHeader.hlsli"

PixelOutput BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1));
	float brightness = dot(-light, input.normal); // �P�ʃx�N�g���Ȃ̂œ��ς����̂܂�Cos�Ƃ̒l�ƂȂ�
	float2 normalUV = (input.normal.xy + float2(1, -1)) * float2(0.5, -0.5);
	float2 sphereMapUV = (input.vnormal.xy + float2(1, -1)) * float2(0.5, -0.5);
	float4 texColor = tex.Sample(smp, input.uv); // �e�N�X�`���J���[
	float4 sphColor = sph.Sample(smp, sphereMapUV);
	float4 spaColor = spa.Sample(smp, sphereMapUV);

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

	float4 ret = max(saturate(
		toonDif // �f�B�t���[�Y�F
		* diffuse
		* texColor
		* sphColor)
		+ saturate(spaColor
		+ float4(specularB * specular.rgb, 1)) // �X�y�L����
		, float4(ambient * texColor, 1)); // �A���r�G���g

	PixelOutput output;
	output.col = float4(ret.rgb * shadowWeight, ret.a);
	output.normal.rgb = float3((input.normal.xyz + 1.0) / 2.0);
	output.normal.a = 1.0;
	float y = dot(float3(0.299, 0.587, 0.1114), output.col);
	output.highLum = y > 0.99 ? output.col : 0.0;

	// �f�B�t�@�[�h�V�F�[�f�B���O�p�̏���
	{
		//output.col = float4(spaColor + sphColor * texColor * diffuse);
		//output.normal.rgb = float3((input.normal.xyz + 1.0) / 2.0);
		//output.normal.a = 1;
	}

	// �e�`��
	if (input.instNo == 1)
	{
		output.col = float4(0.0, 0.0, 0.0, 0.0);
		return output;
	}

	return output;
}