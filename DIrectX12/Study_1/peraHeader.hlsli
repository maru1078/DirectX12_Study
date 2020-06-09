Texture2D<float4> tex          : register(t0); // 通常テクスチャ
Texture2D<float4> texNormal    : register(t1); // 法線

Texture2D<float4> effectTex    : register(t3); // 歪み用
Texture2D<float> depthTex      : register(t4); // 深度用
Texture2D<float> lightDepthTex : register(t5); // ライトデプス

SamplerState smp : register(s0);      // サンプラー

struct Output
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};

cbuffer PostEffect : register(b0)
{
	float4 bkweights[2];
};