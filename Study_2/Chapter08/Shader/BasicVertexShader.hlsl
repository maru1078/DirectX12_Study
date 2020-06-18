#include "BasicShaderHeader.hlsli"

Output BasicVS(
	float4 pos        : POSITION,
	float4 normal     : NORMAL,
	float2 uv         : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight  : WEIGHT)
{
	Output output;
	output.svpos = mul(world, pos);
	output.svpos = mul(mul(proj, view), output.svpos); // �V�F�[�_�[�ł͗�D��Ȃ̂ŏ��Ԃɒ���
	normal.w = 0; // �����d�v�i���s�ړ������𖳌��ɂ���j
	output.normal = mul(world, normal); // �@���ɂ��ϊ����s��
	output.vnormal = mul(view, output.normal);
	output.uv = uv;
	output.ray = normalize(pos.xyz - eye);

	return output;
}