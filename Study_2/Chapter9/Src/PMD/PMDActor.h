#ifndef PMD_ACTOR_H_
#define PMD_ACTOR_H_

#include <memory>
#include <string>
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace DirectX;

class Dx12Wrapper;

// PMDデータ
struct PMDHeader
{
	float version;       // 例：00 00 80 3F == 1.00
	char model_name[20]; // モデル名
	char comment[256];   // モデルコメント
};

#pragma pack(1)

// PMD頂点データ
struct PMDVertex
{
	XMFLOAT3 pos;             // 頂点座標
	XMFLOAT3 normal;          // 法線ベクトル
	XMFLOAT2 uv;              // uv座標
	unsigned short boneNo[2]; // ボーン番号
	unsigned char boneWeight; // ボーン影響度
	unsigned char edgeFlag;   // 輪郭線フラグ
};

// PMDマテリアル構造体
struct PMDMaterial
{
	XMFLOAT3 diffuse;        // ディフューズ色
	float alpha;             // ディフューズα
	float specularity;       // スペキュラの強さ（乗算値）
	XMFLOAT3 specular;       // スペキュラ色
	XMFLOAT3 ambient;        // アンビエント色
	unsigned char toonIdx;   // トゥーン番号
	unsigned char edgeFlg;   // マテリアルごとの輪郭線フラグ
	unsigned int indicesNum; // このマテリアルが割り当てられるインデックス数
	char texFilePath[20];    // テクスチャファイルパス+α
};

#pragma pack()

// シェーダー側に投げられるマテリアルデータ
struct MaterialForHlsl
{
	XMFLOAT3 diffuse;  // ディフューズ色
	float alpha;	   // ディフューズα
	XMFLOAT3 specular; // スペキュラ色
	float specularity; // スペキュラの強さ（乗算値）
	XMFLOAT3 ambient;  // アンビエント色
};

// それ以外のマテリアルデータ
struct AdditionalMaterial
{
	std::string texPath; // テクスチャファイルパス
	int toonIdx;         // トゥーン番号
	bool edgeFlg;        // マテリアルごとの輪郭線フラグ
};

// 全体をまとめるデータ
struct Material
{
	unsigned int indicesNum; // インデックス数
	MaterialForHlsl material;
	AdditionalMaterial additional;
};

class PMDActor
{
public:

	PMDActor(const std::string& filePath, std::weak_ptr<Dx12Wrapper> dx12);

	void Draw();

private:

	bool LoadPMDFile(const std::string& filePath);
	bool CreateMaterialBuffer();
	bool CreateMaterialBufferView();
	bool CreateVertexBuffer();
	bool CreateIndexBuffer();

private:

	std::weak_ptr<Dx12Wrapper> m_dx12;

	std::vector<unsigned char> vertices;
	std::vector<unsigned short> indices;
	std::vector<Material> materials;

	ComPtr<ID3D12Resource> vertBuff{ nullptr };
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	ComPtr<ID3D12Resource> idxBuff{ nullptr };
	D3D12_INDEX_BUFFER_VIEW ibView{};
	ComPtr<ID3D12Resource> materialBuff{ nullptr };

	std::vector<ComPtr<ID3D12Resource>> texResources;
	std::vector<ComPtr<ID3D12Resource>> sphResources;
	std::vector<ComPtr<ID3D12Resource>> spaResources;
	std::vector<ComPtr<ID3D12Resource>> toonResources;

	ComPtr<ID3D12DescriptorHeap> materialDescHeap{ nullptr };
};

#endif // !PMD_ACTOR_H_

