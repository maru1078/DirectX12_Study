#include "BasicShaderHeader.hlsli"

Output BasicVS(
	float4 pos        : POSITION,
	float4 normal     : NORMAL,
	float2 uv         : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight  : WEIGHT,
	uint instNo       : SV_InstanceID)
{
	float w = weight / 100.0f;
	matrix bm = bones[boneno[0]] * w + bones[boneno[1]] * (1 - w);

	Output output;
	pos = mul(bm, pos);
	output.svpos = mul(world, pos);
	if (instNo == 1)
	{
		output.svpos = mul(shadow, output.svpos);
	}
	output.tpos = mul(lightCamera, output.svpos);
	output.svpos = mul(mul(proj, view), output.svpos); // シェーダーでは列優先なので順番に注意
	//output.svpos = mul(lightCamera, output.svpos);
	normal.w = 0; // ここ重要（平行移動成分を無効にする）
	output.normal = mul(world, normal); // 法線にも変換を行う
	output.vnormal = mul(view, output.normal);
	output.uv = uv;
	output.instNo = instNo;
	output.ray = normalize(pos.xyz - eye);

	return output;
}

float4 ShadowVS(
	float4 pos : POSITION,
	float4 normal : NORMAL,
	float2 uv : TEXCOORD,
	min16uint2 boneno : BONE_NO,
	min16uint weight : WEIGHT) : SV_POSITION
{
	float fWeight = float(weight) / 100.0f;
    matrix conBone = bones[boneno.x] * fWeight
    + bones[boneno.y] * (1.0 - fWeight);

	pos = mul(world, mul(conBone, pos));
	return mul(lightCamera, pos);
}