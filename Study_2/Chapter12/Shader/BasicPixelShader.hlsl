#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
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

	//return float4(brightness, brightness, brightness, 1.0) // �P�x
	//	* diffuse  // �f�B�t���[�Y�F
	//	* tex.Sample(smp, input.uv) // �e�N�X�`���J���[
	//	* sph.Sample(smp, sphereMapUV) // ��Z�X�t�B�A�}�b�v
	//	+ spa.Sample(smp, sphereMapUV) // ���Z�X�t�B�A�}�b�v
	//	+ float4(texColor * ambient, 1.0); // �A���r�G���g

	return max(saturate(
		toonDif // �f�B�t���[�Y�F
		* diffuse
		* texColor
		* sph.Sample(smp, sphereMapUV))  
		+ saturate(spa.Sample(smp, sphereMapUV)
		+ float4(specularB * specular.rgb, 1)) // �X�y�L����
		, float4(ambient * texColor, 1)); // �A���r�G���g
}