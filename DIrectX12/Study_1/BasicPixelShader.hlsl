#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
{
	//return float4(input.uv, 1, 1);

	//return float4(tex.Sample(smp, input.uv));

	//return float4(0, 0, 0, 1);
	//return float4(input.normal.xyz, 1);

	if (input.instNo == 1)
	{
		return float4(0, 0, 0, 1);
	}

	float3 light = normalize(float3(1, -1, 1));// �����O����̌�

	// ���C�g�̃J���[
	float3 lightColor = float3(1, 1, 1);

	// �f�B�t���[�Y�v�Z
	float diffuseB = saturate(dot(-light, input.normal));
	float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	// ���̔��˃x�N�g��
	float3 refLight = normalize(reflect(light, input.normal.xyz)); // ���s�����x�N�g��
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	// �X�t�B�A�}�b�v�puv
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	// �e�N�X�`���J���[
	float4 texColor = tex.Sample(smp, input.uv);


	 float brightness = dot(-light, input.normal);


	return max(saturate(
		toonDif // �P�x�i�g�D�[���j
		* diffuse // �f�B�t���[�Y�F
		* texColor // �e�N�X�`���J���[
		* sph.Sample(smp, sphereMapUV)) // �X�t�B�A�}�b�v�i��Z�j
		+ saturate(spa.Sample(smp, sphereMapUV) * texColor // �X�t�B�A�}�b�v�i���Z�j
		+ float4(specularB * specular.rgb, 1)) // �X�y�L����
		, float4(texColor * ambient, 1) // �A���r�G���g
	);

	//return float4(brightness, brightness, brightness, 1)
	//	* diffuse
	//	* texColor  // �e�N�X�`���J���[
	//	* sph.Sample(smp, sphereMapUV)// �X�t�B�A�}�b�v�i��Z�j
	//	+ spa.Sample(smp, sphereMapUV) // �X�t�B�A�}�b�v�i���Z�j
	//	+ float4(texColor * ambient, 1); // �A���r�G���g
}