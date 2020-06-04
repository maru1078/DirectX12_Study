// 頂点シェーダーからピクセルシェーダーへのやり取りに使用する構造体
struct Output
{
	float4 svpos : SV_POSITION; // システム用頂点座標
	float4 pos : POSITION; // 頂点座標
	float4 normal : NORMAL0; // 法線ベクトル
	float4 vnormal : NORMAL1; // 法線ベクトル
	float2 uv    : TEXCOORD; // uv値
	float3 ray : VECTOR; // ベクトル
	uint instNo : SV_InstanceID;
};

Texture2D<float4> tex : register(t0); // 0番スロットに設定されたテクスチャ
Texture2D<float4> sph : register(t1); // 1番スロットに設定されたテクスチャ
Texture2D<float4> spa : register(t2); // 2番スロットに設定されたテクスチャ
Texture2D<float4> toon : register(t3); // 3番スロットに設定されたテクスチャ（トゥーン）

// 深度検証用
//Texture2D<float> depthTex : register(t2);

SamplerState smp : register(s0);      // 0番スロットに設定されたサンプラー
SamplerState smpToon : register(s1);      // 1番スロットに設定されたサンプラー

// 定数バッファ
cbuffer cbuff0 : register(b0)
{
	matrix view;   // ビュー行列
	matrix proj;   // プロジェクション行列
	matrix lightCamera; // ライトビュープロジェクション
	matrix shadow; // 影行列
	float3 eye;    // 視点
};

cbuffer Transform : register(b1)
{
	matrix world;      // ワールド変換行列
	matrix bones[256]; // ボーン行列
}

// 定数バッファ1
// マテリアル用
cbuffer Material : register(b2)
{
	float4 diffuse;  // ディフューズ色
	float4 specular; // スペキュラ
	float3 ambient;  // アンビエント
};