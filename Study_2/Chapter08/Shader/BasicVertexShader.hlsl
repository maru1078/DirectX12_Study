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
	output.svpos = mul(mul(proj, view), output.svpos); // シェーダーでは列優先なので順番に注意
	normal.w = 0; // ここ重要（平行移動成分を無効にする）
	output.normal = mul(world, normal); // 法線にも変換を行う
	output.vnormal = mul(view, output.normal);
	output.uv = uv;
	output.ray = normalize(pos.xyz - eye);

	return output;
}