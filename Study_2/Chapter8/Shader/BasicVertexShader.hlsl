#include "BasicShaderHeader.hlsli"

Output BasicVS(
	float4 pos        : POSITION,
	float4 normal     : NORMAL,
	float2 uv         : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight  : WEIGHT)
{
	Output output;
	output.svpos = mul(mul(viewproj, world), pos); // �V�F�[�_�[�ł͗�D��Ȃ̂ŏ��Ԃɒ���
	normal.w = 0; // �����d�v�i���s�ړ������𖳌��ɂ���j
	output.normal = mul(world, normal); // �@���ɂ��ϊ����s��
	output.uv = uv;

	return output;
}