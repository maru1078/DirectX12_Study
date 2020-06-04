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

	float3 light = normalize(float3(1, -1, 1));// 左上手前からの光

	// ライトのカラー
	float3 lightColor = float3(1, 1, 1);

	// ディフューズ計算
	float diffuseB = saturate(dot(-light, input.normal));
	float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

	// 光の反射ベクトル
	float3 refLight = normalize(reflect(light, input.normal.xyz)); // 平行光線ベクトル
	float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	// スフィアマップ用uv
	float2 sphereMapUV = input.vnormal.xy;
	sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

	// テクスチャカラー
	float4 texColor = tex.Sample(smp, input.uv);


	 float brightness = dot(-light, input.normal);


	return max(saturate(
		toonDif // 輝度（トゥーン）
		* diffuse // ディフューズ色
		* texColor // テクスチャカラー
		* sph.Sample(smp, sphereMapUV)) // スフィアマップ（乗算）
		+ saturate(spa.Sample(smp, sphereMapUV) * texColor // スフィアマップ（加算）
		+ float4(specularB * specular.rgb, 1)) // スペキュラ
		, float4(texColor * ambient, 1) // アンビエント
	);

	//return float4(brightness, brightness, brightness, 1)
	//	* diffuse
	//	* texColor  // テクスチャカラー
	//	* sph.Sample(smp, sphereMapUV)// スフィアマップ（乗算）
	//	+ spa.Sample(smp, sphereMapUV) // スフィアマップ（加算）
	//	+ float4(texColor * ambient, 1); // アンビエント
}