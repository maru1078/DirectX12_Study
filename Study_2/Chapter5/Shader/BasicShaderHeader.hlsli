// 頂点シェーダーからピクセルシェーダーへのやり取りに使用する構造体
struct Output
{
	float4 svpos : SV_POSITION; // システム用頂点座標
	float2 uv    : TEXCOORD; // uv値（テクスチャ上の座標 0.0f～1.0f）
};