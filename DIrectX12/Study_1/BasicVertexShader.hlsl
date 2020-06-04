#include "BasicShaderHeader.hlsli"

Output BasicVS(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONENO,
	min16uint weight : WEIGHT,
	uint instNo : SV_InstanceID
)
{

	float w = weight / 100.0f;
	matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1.0f - w);

	Output output; // �s�N�Z���V�F�[�_�[�ɓn���l
	output.instNo = instNo;
	output.pos = mul(bm, pos);
	output.pos = mul(world, output.pos);
	// �V���h�E�s��Ōv�Z
	if (instNo == 1)
	{
		output.pos = mul(shadow, output.pos);
	}
    //output.svpos = mul(mul(mul(proj, view), world), output.pos);
	output.svpos = mul(lightCamera, output.pos);
	normal.w = 0; // �������d�v�i���s�ړ������𖳌��ɂ���j
	output.normal = mul(world, normal); // �@���ɂ����[���h�ϊ����s��
	output.vnormal = mul(view, output.normal);
	output.uv = uv;
	output.ray = normalize(pos.xyz - eye);

	return output;
}