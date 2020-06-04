#pragma once

#include <d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<wrl.h>
#include <map>
#include <unordered_map>
#include <memory>

using namespace Microsoft::WRL;
using namespace DirectX;

class Dx12Wrapper;

class PMDActor
{
private:

	std::shared_ptr<Dx12Wrapper> m_dx12;

	// 頂点関連
	ComPtr<ID3D12Resource> m_vb{ nullptr };
	ComPtr<ID3D12Resource> m_ib{ nullptr };
	D3D12_VERTEX_BUFFER_VIEW m_vbView{};
	D3D12_INDEX_BUFFER_VIEW m_ibView{};

	ComPtr<ID3D12Resource> m_transformMat{ nullptr }; // 座標変換行列（今はワールドのみ）
	ComPtr<ID3D12DescriptorHeap> m_transformHeap{ nullptr }; // 座標変換ヒープ

		// シェーダー側に投げられるマテリアルデータ
	struct MaterialForHlsl
	{
		XMFLOAT3 diffuse; // ディフューズ
		float alpha; // ディフューズα
		XMFLOAT3 specular; // スペキュラ色
		float specularity; // スペキュラ色
		XMFLOAT3 ambient; // アンビエント
	};

	// それ以外のマテリアルデータ
	struct AdditionalMaterial
	{
		std::string texPath; // テクスチャファイルパス
		int toonIdx; // トゥーン番号
		bool edgeFlag; // マテリアル毎の輪郭線フラグ
	};

	// まとめたもの
	struct Material
	{
		unsigned int indicesNum; // インデックス数
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	struct Transform
	{
		// 内部に持ってるXMMATRIXメンバが16バイトアラインメントであるため
		// Transformをnewする際には16バイト境界に確保する
		//void* operator new(size_t size);
		XMMATRIX world;
	};

	Transform m_transform;
	Transform* m_mappedTransform{ nullptr };
	ComPtr<ID3D12Resource> m_transformBuffer{ nullptr };

	// マテリアル関連
	std::vector<Material> m_materials;
	ComPtr<ID3D12Resource> m_materialBuff{ nullptr };
	std::vector<ComPtr<ID3D12Resource>> m_textureResources;
	std::vector<ComPtr<ID3D12Resource>> m_sphResources;
	std::vector<ComPtr<ID3D12Resource>> m_spaResources;
	std::vector<ComPtr<ID3D12Resource>> m_toonResources;

	ComPtr<ID3D12DescriptorHeap> m_materialDescHeap{ nullptr };

	// ボーン関連
	struct BoneNode
	{
		uint32_t boneIdx; // ボーンインデックス
		uint32_t boneType; // ボーン種別
		uint32_t parentBone; // 謎？
		uint32_t ikParentBone; // IK親ボーン
		XMFLOAT3 startPos; // ボーン基準点（回転の中心）
		//XMFLOAT3 endPos; // ボーン先端点（実際のスキニングには利用しない）
		std::vector<BoneNode*> children; // 子ノード
	};

	std::vector<XMMATRIX> m_boneMatrices;
	std::map<std::string, BoneNode> m_boneNodeTable;
	XMMATRIX* m_mappedMatrices{ nullptr };

	// インデックスから名前を検索しやすいように
	std::vector<std::string> m_boneNameArray;

	// インデックスからノードを検索しやすいように
	std::vector<BoneNode*> m_boneNodeAddressArray;

	// モーション構造体
	struct Motion
	{
		unsigned int frameNo; // アニメーション開始からのフレーム数
		XMVECTOR quaternion; // クォータニオン
		XMFLOAT3 offset; // IKの初期座標からのオフセット情報
		XMFLOAT2 p1, p2; // ベジェ曲線の中間コントロールポイント

		Motion(unsigned int fno, const XMVECTOR& q, XMFLOAT3& ofst, const XMFLOAT2& ip1, const XMFLOAT2& ip2)
			: frameNo(fno), quaternion(q), offset(ofst), p1(ip1), p2(ip2) {}
	};
	
	std::unordered_map<std::string, std::vector<Motion>> m_motionData;
	DWORD m_startTime;
	unsigned int m_duration{ 0 };


	struct PMDIK
	{
		uint16_t boneIdx;                // IK対象のボーンを示す
		uint16_t targetIdx;              // ターゲットに近づけるためのボーンのインデックス
		uint16_t iterations;             // 試行回数
		float limit;                     // 1回あたりの回転制限
		std::vector<uint16_t> nodeIdxes; // 間のノード番号
	};

	std::vector<PMDIK> m_pmdIkData;
	std::vector<uint32_t> m_kneeIdxes;

	// IKオン/オフデータ
	struct VMDIKEnable
	{
		// キーフレームがあるフレーム番号
		uint32_t frameNo;

		// 名前をオン/オフフラグのマップ
		std::unordered_map<std::string, bool> ikEnableTable;
	};

	std::vector<VMDIKEnable> m_ikEnableData;

	XMFLOAT3 m_pos{ 0.0f, 0.0f, 0.0f };

	float m_angle; // テスト用Y軸回転

	unsigned int m_indicesNum;


private:

	// 読み込んだマテリアルをもとにマテリアルバッファを作成
	HRESULT CreateMaterialData();

	// マテリアル＆テクスチャビューを生成
	HRESULT CreateMaterialAndTextureView();

	// 座標変換用ビューの生成
	HRESULT CreateTransformView();

	// PMDファイルのロード
	HRESULT LoadPMDFile(const char* path);

	void RecursiveMatrixMultiply(BoneNode* node, const XMMATRIX& mat);

	void MotionUpdate();

	float GetYFromXOnBezier(float x, const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n);

	// CCK-IKによりボーン方向を解決
	// @param ik 対象IKオブジェクト
	void SolveCCDIK(const PMDIK& ik);

	// 余弦定理IKによりボーン方向を解決
	// @param ik 対象IKオブジェクト
	void SolveCosineIK(const PMDIK& ik);

	// LookAt行列によりボーン方向を解決
	// @param ik 対象IKオブジェクト
	void SolveLookAt(const PMDIK& ik);

	void IKSolve(int frameNo);

public:

	PMDActor(const char* filePath, std::shared_ptr<Dx12Wrapper> dx12);
	~PMDActor();

	HRESULT LoadVMDFile(const char* path);

	void Update();
	void Draw(bool isShadow = false);
	void PlayAnimation();
	void SetPos(float x, float y, float z);
};