Texture2D<float4> tex              : register(t0); // 通常テクスチャ
Texture2D<float4> texNormal        : register(t1); // 法線テクスチャ
Texture2D<float4> texHighLum       : register(t2); // 高輝度
Texture2D<float4> texShrinkHighLum : register(t3); // 縮小バッファー高輝度
Texture2D<float4> texShrink        : register(t4); // 縮小バッファー通常
Texture2D<float4> effectTex        : register(t5); // 歪み用法線マップ
Texture2D<float> depthTex          : register(t6); // 深度テクスチャ
Texture2D<float> lightDepthTex     : register(t7); // ライトデプス

SamplerState smp : register(s0); // サンプラー

struct Output
{
	float4 svpos : SV_POSITION;
	float2 uv    : TEXCOORD;
};

struct BlurOutput
{
	float4 highLum : SV_TARGET0; // 高輝度（High Luminance
	float4 col     : SV_TARGET1; // 通常のレンダリング結果
};

cbuffer PostEffect : register(b0)
{
	float4 bkweights[2];
};