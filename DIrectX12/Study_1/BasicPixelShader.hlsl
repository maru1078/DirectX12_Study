#include "BasicShaderHeader.hlsli"

PixelOutput BasicPS(Output input)/* : SV_TARGET*/
{
	//return float4(input.uv, 1, 1);

	//return float4(tex.Sample(smp, input.uv));

	//return float4(0, 0, 0, 1);
	//return float4(input.normal.xyz, 1);

	//if (input.instNo == 1)
	//{
	//	return float4(0, 0, 0, 1);
	//}

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
	//float brightness = dot(-light, input.normal);

	float3 posFromLightVP = input.tpos.xyz / input.tpos.w;
	float2 shadowUV = (posFromLightVP + float2(1, -1)) * float2(0.5, -0.5);
	float depthFromLight = lightDepthTex.Sample(smp, shadowUV);
	float shadowWeight = 1.0f;

	if (depthFromLight < posFromLightVP.z)
	{
		shadowWeight = 0.5f;
	}

	// 「輝度にこのshadowWeightを乗算して」って書いてあるけど・・・？
	//toonDif *= shadowWeight;

	// いつの間にかサンプルと違くなってる・・・(´;ω;｀)
	float4 ret = max(saturate(
		toonDif // 輝度（トゥーン）
		* diffuse // ディフューズ色
		* texColor // テクスチャカラー
		* sph.Sample(smp, sphereMapUV)) // スフィアマップ（乗算）
		+ saturate(spa.Sample(smp, sphereMapUV) * texColor // スフィアマップ（加算）
			+ float4(specularB * specular.rgb, 1)) // スペキュラ
		, float4(texColor * ambient, 1) // アンビエント
	);

	PixelOutput output;
	output.col = float4(ret.rgb * shadowWeight, ret.a);
	//output.col = float4((input.normal.xyz + 1.0) / 2.0, 1);
	output.normal.rgb = float3((input.normal.xyz + 1.0f) / 2.0f);
	output.normal.a = 1;
	output.highLum = (ret > 1.0f);

	// ディファードシェーディング
	{
     //   float2 spUV = (input.normal.xy * float2(1, -1) + float2(1, 1)) / 2;
	    //float4 sphCol = sph.Sample(smp, spUV);
	    //float4 spaCol = spa.Sample(smp, spUV);
	    //float4 texCol = tex.Sample(smp, input.uv);
	    //
	    //output.col = float4(spaCol + sphCol * texCol * diffuse);
	}
	

	if (input.instNo == 1)
	{
		output.col = float4(0, 0, 0, 1);
		output.normal = float4(0, 0, 0, 1);
		return output;
	}

	return output;

	//return float4(ret.rgb * shadowWeight, ret.a);

	//return float4(brightness, brightness, brightness, 1)
	//	* diffuse
	//	* texColor  // テクスチャカラー
	//	* sph.Sample(smp, sphereMapUV)// スフィアマップ（乗算）
	//	+ spa.Sample(smp, sphereMapUV) // スフィアマップ（加算）
	//	+ float4(texColor * ambient, 1); // アンビエント
}