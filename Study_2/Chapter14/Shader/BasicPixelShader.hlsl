#include "BasicShaderHeader.hlsli"

PixelOutput BasicPS(Output input) : SV_TARGET
{
	float3 light = normalize(float3(1, -1, 1));
	float brightness = dot(-light, input.normal); // 単位ベクトルなので内積がそのままCosθの値となる
	float2 normalUV = (input.normal.xy + float2(1, -1)) * float2(0.5, -0.5);
	float2 sphereMapUV = (input.vnormal.xy + float2(1, -1)) * float2(0.5, -0.5);
	float4 texColor = tex.Sample(smp, input.uv); // テクスチャカラー

	// ディフューズ計算
	float diffuseB = saturate(dot(-light, input.normal));
	float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	// 光の反射ベクトル
	float3 refLight = normalize(reflect(light, input.normal.xyz));
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	float3 posFromLightVP = input.tpos.xyz / input.tpos.w;
	float2 shadowUV = (posFromLightVP + float2(1, -1)) * float2(0.5, -0.5);
	float depthFromLight = lightDepthTex.SampleCmp(
		shadowSmp, // 比較サンプラー
		shadowUV, // uv値
		posFromLightVP.z - 0.005); // 比較対象値
	float shadowWeight = lerp(0.5, 1.0, depthFromLight);
	//if (depthFromLight < posFromLightVP.z - 0.001)
	//{
	//	shadowWeight = 0.5;
	//}

	float4 ret = max(saturate(
		toonDif // ディフューズ色
		* diffuse
		* texColor
		* sph.Sample(smp, sphereMapUV))  
		+ saturate(spa.Sample(smp, sphereMapUV)
		+ float4(specularB * specular.rgb, 1)) // スペキュラ
		, float4(ambient * texColor, 1)); // アンビエント

	PixelOutput output;
	output.col = float4(ret.rgb * shadowWeight, ret.a);
	output.normal.rgb = float3((input.normal.xyz + 1.0) / 2.0);
	output.normal.a = 1.0;

	// 影描画
	if (input.instNo == 1)
	{
		output.col = float4(0.0, 0.0, 0.0, 0.0);
		return output;
	}

	return output;
}