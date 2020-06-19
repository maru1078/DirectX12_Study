#ifndef PMD_ACTOR_H_
#define PMD_ACTOR_H_

#include <memory>
#include <map>
#include <unordered_map>
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

// 読み込み用ボーン構造体
struct PMDBone
{
	char boneName[20];       // ボーン名
	unsigned short parentNo; // 親ボーン番号
	unsigned short nextNo;   // 先端のボーン番号
	unsigned char type;      // ボーン種別
	unsigned short ikBoneNo; // IKボーン番号
	XMFLOAT3 pos;            // ボーンの基準点座標
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

struct BoneNode
{
	int boneIdx;					 // ボーンインデックス
	XMFLOAT3 startPos;				 // ボーン基準点（回転の中心）
	XMFLOAT3 endPos;				 // ボーン先端点（実際のスキニングには利用しない）
	std::vector<BoneNode*> children; // 子ノード
};

struct VMDMotion
{
	char boneName[15];		  // ボーン名
	unsigned int frameNo;	  // フレーム番号
	XMFLOAT3 location;		  // 位置
	XMFLOAT4 quaternion;	  // クォータニオン（回転）
	unsigned char bezier[64]; // [4][4][4]ベジェ補間パラメーター
};

struct Motion
{
	unsigned int frameNo;
	XMVECTOR quaternion;

	Motion(unsigned int fno, const XMVECTOR& q)
		: frameNo{ fno }
		, quaternion{ q }
	{}
};

class PMDActor
{
public:

	PMDActor(const std::string& filePath, 
		std::weak_ptr<Dx12Wrapper> dx12,
		XMFLOAT3 pos = { 0.0f, 0.0f, 0.0f });

	void PlayAnimation();
	void MotionUpdate();
	void Update();
	void Draw();

private:

	bool LoadPMDFile(const std::string& filePath);
	bool LoadVMDFile(const std::string& filePath);
	bool CreateMaterialBuffer();
	bool CreateMaterialBufferView();
	bool CreateVertexBuffer();
	bool CreateIndexBuffer();
	bool CreateTransformBuffer();
	bool CreateTransformBufferView();

	void RecursiveMatrixMultiply(BoneNode* node, const XMMATRIX& mat);

private:

	XMFLOAT3 m_pos{ 0.0f, 0.0f, 0.0f };
	float m_angle{ 0.0f };

	std::weak_ptr<Dx12Wrapper> m_dx12;

	std::vector<unsigned char> m_vertices;
	std::vector<unsigned short> m_indices;
	std::vector<Material> m_materials;

	ComPtr<ID3D12Resource> m_vertBuff{ nullptr };
	D3D12_VERTEX_BUFFER_VIEW m_vbView{};
	ComPtr<ID3D12Resource> m_idxBuff{ nullptr };
	D3D12_INDEX_BUFFER_VIEW m_ibView{};
	ComPtr<ID3D12Resource> m_materialBuff{ nullptr };

	std::vector<ComPtr<ID3D12Resource>> m_texResources;
	std::vector<ComPtr<ID3D12Resource>> m_sphResources;
	std::vector<ComPtr<ID3D12Resource>> m_spaResources;
	std::vector<ComPtr<ID3D12Resource>> m_toonResources;

	ComPtr<ID3D12DescriptorHeap> m_materialDescHeap{ nullptr };

	XMMATRIX* m_mappedMatrices{ nullptr };
	ComPtr<ID3D12Resource> m_transformBuffer{ nullptr };
	ComPtr<ID3D12DescriptorHeap> m_transformHeap{ nullptr };
	
	std::vector<XMMATRIX> m_boneMatrices;
	std::map<std::string, BoneNode> m_boneNodeTable;

	std::unordered_map<std::string, std::vector<Motion>> m_motionData;
	DWORD m_startTime; // アニメーション開始時のミリ秒
};

#endif // !PMD_ACTOR_H_

