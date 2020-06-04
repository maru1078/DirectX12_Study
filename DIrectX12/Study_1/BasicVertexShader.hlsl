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

	Output output; // ピクセルシェーダーに渡す値
	output.instNo = instNo;
	output.pos = mul(bm, pos);
	output.pos = mul(world, output.pos);
	// シャドウ行列で計算
	if (instNo == 1)
	{
		output.pos = mul(shadow, output.pos);
	}
    //output.svpos = mul(mul(mul(proj, view), world), output.pos);

	// サンプルがこうなってたが、モデルの間隔が狭い気がする。
	output.svpos = mul(proj, mul(view, output.pos));

	//// ライトからの平行投影
	//output.svpos = mul(lightCamera, output.pos);
	output.tpos = mul(lightCamera, output.pos);
	normal.w = 0; // ここが重要（平行移動成分を無効にする）
	output.normal = mul(world, normal); // 法線にもワールド変換を行う
	output.vnormal = mul(view, output.normal);
	output.uv = uv;
	output.ray = normalize(pos.xyz - eye);

	return output;
}

// 影用頂点座標変換（座標変換のみ）
float4 ShadowVS(float4 pos        : POSITION,
	            float4 normal     : NORMAL,
	            float2 uv         : TEXCOORD,
	            min16uint2 boneno : BONENO,
	            min16uint weight  : WEIGHT) : SV_POSITION
{
	float fWeight = float(weight) / 100.0f;
    matrix conBone = bones[boneno.x]
    * fWeight
    + bones[boneno.y]
    * (1.0f - fWeight);

	pos = mul(world, mul(conBone, pos));

	return mul(lightCamera, pos);
}