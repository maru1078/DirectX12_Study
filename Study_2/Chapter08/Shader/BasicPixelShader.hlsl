#include "BasicShaderHeader.hlsli"

float4 BasicPS(Output input) : SV_TARGET
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

	//return float4(brightness, brightness, brightness, 1.0) // 輝度
	//	* diffuse  // ディフューズ色
	//	* tex.Sample(smp, input.uv) // テクスチャカラー
	//	* sph.Sample(smp, sphereMapUV) // 乗算スフィアマップ
	//	+ spa.Sample(smp, sphereMapUV) // 加算スフィアマップ
	//	+ float4(texColor * ambient, 1.0); // アンビエント

	return max(saturate(
		toonDif // ディフューズ色
		* diffuse
		* texColor
		* sph.Sample(smp, sphereMapUV))  
		+ saturate(spa.Sample(smp, sphereMapUV)
		+ float4(specularB * specular.rgb, 1)) // スペキュラ
		, float4(ambient * texColor, 1)); // アンビエント
}