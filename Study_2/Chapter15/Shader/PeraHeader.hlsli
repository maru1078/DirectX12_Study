Texture2D<float4> tex              : register(t0); // 通常テクスチャ
Texture2D<float4> texNormal        : register(t1); // 法線テクスチャ
Texture2D<float4> texHighLum       : register(t2); // 高輝度
Texture2D<float4> texShrinkHighLum : register(t3); // 縮小バッファー高輝度
Texture2D<float4> texShrink        : register(t4); // 縮小バッファー通常

Texture2D<float4> effectTex        : register(t5); // 歪み用法線マップ
Texture2D<float> depthTex          : register(t6); // 深度テクスチャ

Texture2D<float> lightDepthTex     : register(t7); // ライトデプス

Texture2D<float> SSAOTex           : register(t8); // SSAO

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

// 定数バッファ
cbuffer SceneMatrix : register(b1)
{
	matrix view;        // ビュー変換行列
	matrix proj;        // プロジェクション変換行列
	matrix invproj;     // 逆プロジェクション
	matrix lightCamera; // ライトビュープロジェクション
	matrix shadow;      // 影
	float3 eye;         // 視点
};