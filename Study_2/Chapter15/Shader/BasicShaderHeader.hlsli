Texture2D<float4> tex : register(t0);  // 0番スロットに設定されたテクスチャ
Texture2D<float4> sph : register(t1);  // 1番スロットに設定されたテクスチャ
Texture2D<float4> spa : register(t2);  // 2番スロットに設定されたテクスチャ
Texture2D<float4> toon : register(t3); // 3番スロットに設定されたテクスチャ
Texture2D<float> lightDepthTex : register(t4); // シャドウマップ用ライト深度テクスチャ

SamplerState smp : register(s0);     // 0番スロットに設定されたサンプラー
SamplerState smpToon : register(s1); // 1番スロットに設定されたサンプラー（トゥーン用）
SamplerComparisonState shadowSmp : register(s2); // シャドウマップ用のサンプラー

// 頂点シェーダーからピクセルシェーダーへのやり取りに使用する構造体
struct Output
{
	float4 svpos   : SV_POSITION; // システム用頂点座標
	float4 normal  : NORMAL0;     // 法線ベクトル
	float4 vnormal : NORMAL1;     // ビュー変換後の法線ベクトル
	float2 uv      : TEXCOORD;    // uv値（テクスチャ上の座標 0.0f～1.0f）
	uint instNo    : SV_InstanceID;
	float3 ray     : VECTOR;      // ベクトル
	float4 tpos    : TPOS;
};

struct PixelOutput
{
	float4 col     : SV_TARGET0;    // カラー値を出力
	float4 normal  : SV_TARGET1; // 法線を出力
	float4 highLum : SV_TARGET2; // 高輝度
};

// 定数バッファ
cbuffer cbuff0 : register(b0)
{
	matrix view;        // ビュー変換行列
	matrix proj;        // プロジェクション変換行列
	matrix invproj;     // 逆プロジェクション
	matrix lightCamera; // ライトビュープロジェクション
	matrix shadow;      // 影
	float3 eye;         // 視点
};

cbuffer Transform : register(b1)
{
	matrix world;     // ワールド変換行列
	matrix bones[256]; // ボーン行列
};

cbuffer Material : register(b2)
{
	float4 diffuse;  // ディフューズ色
	float4 specular; // スペキュラ
	float3 ambient;	 // アンビエント
};