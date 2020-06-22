Texture2D<float4> tex          : register(t0); // 通常テクスチャ
Texture2D<float4> texNormal    : register(t1); // 法線テクスチャ
Texture2D<float4> effectTex    : register(t2); // 歪み用法線マップ
Texture2D<float> depthTex      : register(t3); // 深度テクスチャ
Texture2D<float> lightDepthTex : register(t4); // ライトデプス
SamplerState smp : register(s0); // サンプラー

struct Output
{
	float4 svpos : SV_POSITION;
	float2 uv    : TEXCOORD;
};

cbuffer PostEffect : register(b0)
{
	float4 bkweights[2];
};