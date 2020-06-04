#include "PMDActor.h"

#include "PMDRenderer.h"
#include "../Dx12Wrapper/Dx12Wrapper.h"
#include "../Helper/Helper.h"

#include <array>

#include <algorithm>
#pragma comment(lib, "winmm.lib")

#include <iostream>
#include <sstream>

namespace // 無名名前空間
{
	// ボーン種別
	enum class BoneType
	{
		Rotation,      // 回転
		RotAndMove,    // 回転＆移動
		IK,            // IK
		Undefined,     // 未定義
		IKChild,       // IK影響ボーン
		RotationChild, // 回転影響ボーン
		IKDestination, // IK接続先ボーン
		Invisible      // 見えないボーン
	};

	// z軸を特定の方向に向ける行列を返す関数
	// @param lookat 向かせたい方向ベクトル
	// @param up 上ベクトル
	// @pamra right 右ベクトル
	XMMATRIX LookAtMatrix(const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right)
	{
		// 向かせたい方向（z軸）
		XMVECTOR vz = lookat;

		// （向かせたい方向を向かせた時の）仮のy軸ベクトル
		XMVECTOR vy = XMVector3Normalize(XMLoadFloat3(&up));

		// （向かせたい方向を向かせた時の）y軸
		//XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vz, vx));
		XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));
		vy = XMVector3Normalize(XMVector3Cross(vz, vx));

		// LookAtとupが同じ方向を向いていたらrightを基準にして作り直す
		if (std::abs(XMVector3Dot(vy, vz).m128_f32[0] == 1.0f))
		{
			// 仮のx方向を定義
			vx = XMVector3Normalize(XMLoadFloat3(&right));

			// 向かせたい方向を向かせた時のy軸を計算
			vy = XMVector3Normalize(XMVector3Cross(vz, vx));

			// 真のx軸を計算
			vx = XMVector3Normalize(XMVector3Cross(vy, vz));
		}

		XMMATRIX ret = XMMatrixIdentity();
		ret.r[0] = vx;
		ret.r[1] = vy;
		ret.r[2] = vz;

		return ret;
	}

	// 特定のベクトルを特定の方向に向けるための行列を返す
	// @param origin 特定のベクトル
	// @param lookat 向かせたい方向
	// @param up 上ベクトル
	// @param right 右ベクトル
	// @retbal 特定のベクトルを特定の方向に向かせるための行列
	XMMATRIX LookAtMatrix(const XMVECTOR& origin, const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right)
	{
		return XMMatrixTranspose(LookAtMatrix(origin, up, right)) * LookAtMatrix(lookat, up, right);
	}
}

HRESULT PMDActor::CreateMaterialData()
{
	// マテリアルバッファーを作成
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	auto result = m_dx12->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * m_materials.size()), // もったいないが仕方ない
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_materialBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// マップマテリアルにコピー
	char* mapMaterial = nullptr;
	result = m_materialBuff->Map(0, nullptr, (void**)&mapMaterial);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}


	for (auto& m : m_materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material; // データコピー
		mapMaterial += materialBuffSize; // 次のアラインメント位置まで進める（256の倍数）
	}

	m_materialBuff->Unmap(0, nullptr);

	return result;
}

HRESULT PMDActor::CreateMaterialAndTextureView()
{
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = m_materials.size() * 5; // マテリアル数を指定
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = m_dx12->Device()->CreateDescriptorHeap(
		&matDescHeapDesc, IID_PPV_ARGS(m_materialDescHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc{};
	matCBVDesc.BufferLocation = m_materialBuff->GetGPUVirtualAddress();
	matCBVDesc.SizeInBytes = materialBuffSize;

	// 通常テクスチャビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // デフォルト
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1; // ミップマップは使用しないので 1

	// 先頭を記録
	auto matDescHeapH = m_materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto inc = m_dx12->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < m_materials.size(); ++i)
	{
		// マテリアル用定数バッファービュー
		m_dx12->Device()->CreateConstantBufferView(&matCBVDesc, matDescHeapH);

		matDescHeapH.ptr += inc;
		matCBVDesc.BufferLocation += materialBuffSize;

		// シェーダーリソースビュー（追加）
		if (m_textureResources[i] == nullptr)
		{
			srvDesc.Format = m_dx12->WhiteTexture()->GetDesc().Format;
			m_dx12->Device()->CreateShaderResourceView(
				m_dx12->WhiteTexture().Get(), &srvDesc, matDescHeapH
			);
		}
		else
		{
			srvDesc.Format = m_textureResources[i]->GetDesc().Format;
			m_dx12->Device()->CreateShaderResourceView(
				m_textureResources[i].Get(),
				&srvDesc,
				matDescHeapH
			);
		}

		matDescHeapH.ptr += inc;

		if (m_sphResources[i] == nullptr)
		{
			srvDesc.Format = m_dx12->WhiteTexture()->GetDesc().Format;
			m_dx12->Device()->CreateShaderResourceView(
				m_dx12->WhiteTexture().Get(), &srvDesc, matDescHeapH
			);
		}
		else
		{
			srvDesc.Format = m_sphResources[i]->GetDesc().Format;
			m_dx12->Device()->CreateShaderResourceView(
				m_sphResources[i].Get(),
				&srvDesc,
				matDescHeapH
			);
		}

		matDescHeapH.ptr += inc;

		if (m_spaResources[i] == nullptr)
		{
			srvDesc.Format = m_dx12->BlackTexture()->GetDesc().Format;
			m_dx12->Device()->CreateShaderResourceView(
				m_dx12->BlackTexture().Get(), &srvDesc, matDescHeapH
			);
		}
		else
		{
			srvDesc.Format = m_spaResources[i]->GetDesc().Format;
			m_dx12->Device()->CreateShaderResourceView(
				m_spaResources[i].Get(),
				&srvDesc,
				matDescHeapH
			);
		}

		matDescHeapH.ptr += inc;

		if (m_toonResources[i] == nullptr)
		{
			srvDesc.Format = m_dx12->GradTexture()->GetDesc().Format;
			m_dx12->Device()->CreateShaderResourceView(
				m_dx12->GradTexture().Get(),
				&srvDesc,
				matDescHeapH
			);
		}
		else
		{
			srvDesc.Format = m_toonResources[i]->GetDesc().Format;
			m_dx12->Device()->CreateShaderResourceView(
				m_toonResources[i].Get(),
				&srvDesc,
				matDescHeapH
			);
		}

		matDescHeapH.ptr += inc;
	}

	return result;
}

HRESULT PMDActor::CreateTransformView()
{
	// GPUバッファ作成
	//auto buffSize = sizeof(Transform);
	auto buffSize = sizeof(XMMATRIX) * (1 + m_boneMatrices.size());
	buffSize = (buffSize + 0xff) & ~0xff;
	auto result = m_dx12->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(buffSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_transformBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// マップとコピー
	result = m_transformBuffer->Map(0, nullptr, (void**)&m_mappedMatrices);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}
	m_mappedMatrices[0] = m_transform.world; // 0番にはワールド変換行列を入れる

	copy(m_boneMatrices.begin(), m_boneMatrices.end(), m_mappedMatrices + 1);

	// ビュー作成
	D3D12_DESCRIPTOR_HEAP_DESC transformDescHeapDesc{};
	transformDescHeapDesc.NumDescriptors = 1; // とりあえずワールド一つ
	transformDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	transformDescHeapDesc.NodeMask = 0;
	transformDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // ディスクリプタヒープ種別

	result = m_dx12->Device()->CreateDescriptorHeap(&transformDescHeapDesc,
		IID_PPV_ARGS(m_transformHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = m_transformBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = buffSize;

	m_dx12->Device()->CreateConstantBufferView(&cbvDesc,
		m_transformHeap->GetCPUDescriptorHandleForHeapStart());

	return S_OK;
}

HRESULT PMDActor::LoadPMDFile(const char * path)
{
	struct PMDHeader
	{
		float version;
		char model_name[20]; // モデル名
		char comment[256]; // モデルコメント
	};

	char signature[3]{}; // シグネチャ
	PMDHeader pmdHeader{};

	std::string strModelPath = path;
	auto fp = fopen(strModelPath.c_str(), "rb");

	if (fp == nullptr)
	{
		assert(0);
		return ERROR_FILE_NOT_FOUND;
	}

	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	// 頂点関係
	unsigned int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, fp);

	const unsigned int pmdvertex_size = 38;
	std::vector<unsigned char> vertices(vertNum * pmdvertex_size); // バッファーの確保
	fread(vertices.data(), vertices.size(), 1, fp);

	// CD3DX～を使った頂点バッファの作成
	auto result = m_dx12->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // UPLOADヒープとして
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertices.size()), // サイズに応じて適切な設定をしてくれる
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_vb.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	unsigned char* vertMap = nullptr;

	result = m_vb->Map(0, nullptr, (void**)&vertMap);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	std::copy(vertices.begin(), vertices.end(), vertMap);
	m_vb->Unmap(0, nullptr);

	m_vbView.BufferLocation = m_vb->GetGPUVirtualAddress(); // バッファーの仮想アドレス
	m_vbView.SizeInBytes = vertices.size(); // 全バイト数
	m_vbView.StrideInBytes = pmdvertex_size; // 1頂点当たりのバイト数（今回はXMFLOAT3を入れても動くはず）

	// インデックス関係
	fread(&m_indicesNum, sizeof(m_indicesNum), 1, fp);

	std::vector<unsigned short> indices(m_indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	result = m_dx12->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_ib.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// 作ったバッファーにインデックスデータをコピー
	unsigned short* mappedIdx = nullptr;
	m_ib->Map(0, nullptr, (void**)&mappedIdx);

	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	m_ib->Unmap(0, nullptr);

	m_ibView.BufferLocation = m_ib->GetGPUVirtualAddress();
	m_ibView.Format = DXGI_FORMAT_R16_UINT; // unsigned intのインデックス配列だからこの設定
	m_ibView.SizeInBytes = indices.size() * sizeof(indices[0]);

	// マテリアル関係
#pragma pack(1) // ここから1バイトパッキングになり、アラインメントは発生しない

	// PMD マテリアル構造体
	struct PMDMaterial
	{
		XMFLOAT3 diffuse; //　ディフューズ色
		float alpha; // ディフューズα
		float specularity; // スペキュラの強さ
		XMFLOAT3 specular; // スペキュラ色
		XMFLOAT3 ambient; // アンビエント色
		unsigned char toonIdx; // トゥーン番号
		unsigned char edgeFlag; // マテリアルごとの輪郭線フラグ

		// 注意：ここに2バイトのパディングがある！！

		// 1バイトパッキングによってパディングが発生しない！！

		unsigned int indicesNum; // このマテリアルが割り当てられるインデックス数

		char texFilePath[20]; // テクスチャファイルパス + α
	}; // 70バイトのはずだが、パディングが発生するため72バイトになる

#pragma pack() // パッキング指定を解除（デフォルトに戻す）

	unsigned int materialNum;
	// マテリアル読み込み
	fread(&materialNum, sizeof(materialNum), 1, fp);
	m_materials.resize(materialNum);
	m_toonResources.resize(materialNum);
	m_textureResources.resize(materialNum);
	m_sphResources.resize(materialNum);
	m_spaResources.resize(materialNum);

	std::vector<PMDMaterial> pmdMaterials(materialNum);

	fread(
		pmdMaterials.data(),
		pmdMaterials.size() * sizeof(PMDMaterial),
		1, fp);

	// コピー
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		m_materials[i].indicesNum = pmdMaterials[i].indicesNum;
		m_materials[i].material.diffuse = pmdMaterials[i].diffuse;
		m_materials[i].material.alpha = pmdMaterials[i].alpha;
		m_materials[i].material.specular = pmdMaterials[i].specular;
		m_materials[i].material.specularity = pmdMaterials[i].specularity;
		m_materials[i].material.ambient = pmdMaterials[i].ambient;
		m_materials[i].additional.toonIdx = pmdMaterials[i].toonIdx;
	}

	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		// トゥーンリソースの読み込み
		char toonFilePath[32];

		sprintf(
			toonFilePath,
			"toon/toon%02d.bmp",
			pmdMaterials[i].toonIdx + 1);
		m_toonResources[i] = m_dx12->GetTextureByPath(toonFilePath);

		if (strlen(pmdMaterials[i].texFilePath) == 0)
		{
			m_textureResources[i] = nullptr;
			continue;
		}

		std::string texFileName = pmdMaterials[i].texFilePath;
		std::string sphFileName = "";
		std::string spaFileName = "";

		if (std::count(texFileName.begin(), texFileName.end(), '*') > 0)
		{ // スプリッタがある
			auto namepair = SplitFileName(texFileName);
			if (GetExtension(namepair.first) == "sph")
			{
				texFileName = namepair.second;
				sphFileName = namepair.first;
			}
			else if (GetExtension(namepair.first) == "spa")
			{
				texFileName = namepair.second;
				spaFileName = namepair.first;
			}
			else
			{
				texFileName = namepair.first;

				if (GetExtension(namepair.second) == "sph")
				{
					sphFileName = namepair.second;
				}
				else if (GetExtension(namepair.second) == "spa")
				{
					spaFileName = namepair.second;
				}
			}
		}
		else
		{
			if (GetExtension(texFileName) == "sph")
			{
				sphFileName = texFileName;
				texFileName = "";
			}
			else if (GetExtension(texFileName) == "spa")
			{
				spaFileName = texFileName;
				texFileName = "";
			}
		}

		if (strlen(pmdMaterials[i].texFilePath) == 0)
		{
			m_textureResources[i] = nullptr;
		}

		if (texFileName != "")
		{
			// モデルとテクスチャパスからアプリケーションからのテクスチャパスを得る
			auto texFilePath = GetTexturePathFromModelAndTexPath(
				strModelPath, texFileName.c_str());

			m_textureResources[i] = m_dx12->GetTextureByPath(texFilePath.c_str());
		}

		if (sphFileName != "")
		{
			auto sphFilePath = GetTexturePathFromModelAndTexPath(
				strModelPath, sphFileName.c_str());

			m_sphResources[i] = m_dx12->GetTextureByPath(sphFilePath.c_str());
		}

		if (spaFileName != "")
		{
			auto spaFilePath = GetTexturePathFromModelAndTexPath(
				strModelPath, spaFileName.c_str());

			m_spaResources[i] = m_dx12->GetTextureByPath(spaFilePath.c_str());
		}
	}

#pragma pack(1)

	// 読み込み用ボーン構造体
	struct PMDBone
	{
		char boneName[20]; // ボーン名
		unsigned short parentNo; // 親ボーン名
		unsigned short nextNo; // 先端ボーン番号
		unsigned char type; // ボーン種別
		unsigned short ikBoneNo; // IKボーン番号
		XMFLOAT3 pos; // ボーンの基準点座標
	};

#pragma pack()

	unsigned short boneNum = 0;
	fread(&boneNum, sizeof(boneNum), 1, fp);

	std::vector<PMDBone> pmdBones(boneNum);
	fread(pmdBones.data(), sizeof(PMDBone), boneNum, fp);

	// インデックスと名前の対応関係構築のためにあとで使う
	std::vector<std::string> boneNames(pmdBones.size());

	// IK情報の読み込み
	uint16_t ikNum = 0;
	fread(&ikNum, sizeof(ikNum), 1, fp);

	m_pmdIkData.resize(ikNum);

	for (auto& ik : m_pmdIkData)
	{
		fread(&ik.boneIdx, sizeof(ik.boneIdx), 1, fp);
		fread(&ik.targetIdx, sizeof(ik.targetIdx), 1, fp);

		uint8_t chainLen = 0; // 間にいくつノードがあるか
		fread(&chainLen, sizeof(chainLen), 1, fp);

		ik.nodeIdxes.resize(chainLen);

		fread(&ik.iterations, sizeof(ik.iterations), 1, fp);
		fread(&ik.limit, sizeof(ik.limit), 1, fp);

		if (chainLen == 0)
		{
			continue; // 間のノード数が0ならばここで終わり
		}

		fread(ik.nodeIdxes.data(), sizeof(ik.nodeIdxes[0]), chainLen, fp);
	}

	fclose(fp);

	// 読み込み後の処理

	m_boneNameArray.resize(pmdBones.size());
	m_boneNodeAddressArray.resize(pmdBones.size());

	m_boneMatrices.resize(pmdBones.size());

	// ボーンをすべて初期化する
	std::fill(
		m_boneMatrices.begin(),
		m_boneMatrices.end(),
		XMMatrixIdentity());

	m_kneeIdxes.clear();
	// ボーンノードマップを作る
	for (int i = 0; i < pmdBones.size(); ++i)
	{
		auto& pb = pmdBones[i];
		boneNames[i] = pb.boneName;
		auto& node = m_boneNodeTable[pb.boneName];
		node.boneIdx = i;
		node.startPos = pb.pos;
		node.boneType = pb.type;
		node.parentBone = pb.parentNo;
		node.ikParentBone = pb.ikBoneNo;

		// インデックスから検索しやすいように
		m_boneNameArray[i] = pb.boneName;
		m_boneNodeAddressArray[i] = &node;

		std::string boneName = pb.boneName;
		if (boneName.find("ひざ") != std::string::npos)
		{
			m_kneeIdxes.emplace_back(i);
		}
	}

	// 親子関係を構築
	for (auto& pb : pmdBones)
	{
		// 親インデックスをチェック（ありえない番号なら飛ばす）
		if (pb.parentNo >= pmdBones.size())
		{
			continue;
		}

		auto parentName = boneNames[pb.parentNo];
		m_boneNodeTable[parentName].children.emplace_back(&m_boneNodeTable[pb.boneName]);
	}

	// デバッグ用
	auto getNameFromIdx = [&](uint16_t idx)->std::string
	{
		auto it = find_if(m_boneNodeTable.begin(),
			m_boneNodeTable.end(),
			[idx](const std::pair<std::string, BoneNode>& obj)
		{
			return obj.second.boneIdx == idx;
		});

		if (it != m_boneNodeTable.end())
		{
			return it->first;
		}
		else
		{
			return "";
		}
	};

	//for (auto& ik : m_pmdIkData)
	//{
	//	std::ostringstream oss;
	//	oss << "IKボーン番号 = " << ik.boneIdx << " : " << getNameFromIdx(ik.boneIdx) << std::endl;

	//	for (auto& node : ik.nodeIdxes)
	//	{
	//		oss << "\tノードボーン = " << node << " : " << getNameFromIdx(node) << std::endl;
	//	}

	//	std::cout << oss.str().c_str() << std::endl;
	//}

	return result;
}

void PMDActor::RecursiveMatrixMultiply(BoneNode * node, const XMMATRIX & mat)
{
	m_boneMatrices[node->boneIdx] *= mat;

	for (auto& cnode : node->children)
	{
		RecursiveMatrixMultiply(cnode, m_boneMatrices[node->boneIdx]);
	}
}

void PMDActor::MotionUpdate()
{
	auto elaspedTime = timeGetTime() - m_startTime;

	unsigned int frameNo = 30 * (elaspedTime / 1000.0f);

	// ループのための追加コード
	if (frameNo > m_duration)
	{
		m_startTime = timeGetTime();
		frameNo = 0;
	}


	// 行列情報クリア
	// （クリアしていないと前フレームのポーズが重ね掛けされて
	// モデルが壊れる）
	std::fill(m_boneMatrices.begin(), m_boneMatrices.end(), XMMatrixIdentity());

	// モーションデータ更新
	for (auto& boneMotion : m_motionData)
	{
		auto node = m_boneNodeTable[boneMotion.first];

		// 合致するものを探す
		auto motions = boneMotion.second;
		auto rit = std::find_if(
			motions.rbegin(), motions.rend(),
			[frameNo](const Motion& motion) // ラムダ式
		{
			return motion.frameNo <= frameNo;
		});

		// 合致するものがなければ処理を飛ばす
		if (rit == motions.rend())
		{
			continue;
		}

		XMMATRIX rotation;
		XMVECTOR offset = XMLoadFloat3(&rit->offset);
		auto it = rit.base();

		if (it != motions.end())
		{
			auto t = static_cast<float>(frameNo - rit->frameNo) / static_cast<float>(it->frameNo - rit->frameNo);

			t = GetYFromXOnBezier(t, it->p1, it->p2, 12);

			rotation = XMMatrixRotationQuaternion(
				XMQuaternionSlerp(rit->quaternion, it->quaternion, t));

			offset = XMVectorLerp(offset, XMLoadFloat3(&it->offset), t);
		}
		else
		{
			rotation = XMMatrixRotationQuaternion(rit->quaternion);
		}

		auto& pos = node.startPos;

		auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* rotation
			* XMMatrixTranslation(pos.x, pos.y, pos.z);

		m_boneMatrices[node.boneIdx] = mat * XMMatrixTranslationFromVector(offset);
	}

	RecursiveMatrixMultiply(&m_boneNodeTable["センター"], XMMatrixIdentity());

	IKSolve(frameNo);

	copy(m_boneMatrices.begin(), m_boneMatrices.end(), m_mappedMatrices + 1);

}

// 誤差の範囲内かどうかに使用する定数
constexpr float epsilon = 0.0005f;

float PMDActor::GetYFromXOnBezier(float x, const XMFLOAT2 & a, const XMFLOAT2 & b, uint8_t n)
{
	if (a.x == a.y && b.x == b.y)
	{
		return x; // 計算不要
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x; // t~3の係数
	const float k1 = 3 * b.x - 6 * a.x;     // t~2の係数
	const float k2 = 3 * a.x;               // tの係数

	// tを近似で求める
	for (int i = 0; i < n; ++i)
	{
		// f(t)を求める
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;

		// もし結果が0に近い（誤差の範囲内）なら打ち切る
		if (ft <= epsilon && ft >= epsilon)
		{
			break;
		}

		t -= ft / 2; // 刻む
	}

	// 求めたい t はすでに求めているので y を計算する
	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.y + 3 * t * r * r * a.y;
}

void PMDActor::SolveCCDIK(const PMDIK & ik)
{
	// ターゲット
	auto targetBoneNode = m_boneNodeAddressArray[ik.boneIdx];
	auto targetOriginPos = XMLoadFloat3(&targetBoneNode->startPos);

	auto parentMat = m_boneMatrices[m_boneNodeAddressArray[ik.boneIdx]->ikParentBone];
	XMVECTOR det;
	auto invParentMat = XMMatrixInverse(&det, parentMat);
	auto targetNextPos = XMVector3Transform(targetOriginPos, m_boneMatrices[ik.boneIdx] * invParentMat);

	// 末端ノード
	std::vector<XMVECTOR> bonePositions;
	auto endPos = XMLoadFloat3(&m_boneNodeAddressArray[ik.targetIdx]->startPos);

	// 中間ノード（ルートを含む）
	for (auto& cidx : ik.nodeIdxes)
	{
		bonePositions.push_back(XMLoadFloat3(&m_boneNodeAddressArray[cidx]->startPos));
	}

	// 回転行列記録
	std::vector<XMMATRIX> mats(bonePositions.size());
	fill(mats.begin(), mats.end(), XMMatrixIdentity());

	auto ikLimit = ik.limit * XM_PI;

	// ikに設定されている試行回数だけ繰り返す
	for (int c = 0; c < ik.iterations; ++c)
	{
		// ターゲットと末端がほぼ一致したら抜ける
		if (XMVector3Length(XMVectorSubtract(endPos, targetNextPos)).m128_f32[0] <= epsilon)
		{
			break;
		}

		// それぞれのボーンをさかのぼりながら
		// 角度制限に引っかからないように曲げていく

		// bonePositionsはCCD_IKにおける各ノードの座標を
		// ベクタ配列にしたものです
		for (int bidx = 0; bidx < bonePositions.size(); ++bidx)
		{
			const auto& pos = bonePositions[bidx];

			// 対象ノードから末端ノードまでと
			// 対象ノードからターゲットまでのベクトル作成
			auto vecToEnd = XMVectorSubtract(endPos, pos); // 末端へ
			auto vecToTarget = XMVectorSubtract(targetNextPos, pos); // ターゲットへ

			// 両方正規化
			vecToEnd = XMVector3Normalize(vecToEnd);
			vecToTarget = XMVector3Normalize(vecToTarget);

			// ほぼ同じベクトルになってしまった場合は
			// 外積炊きないため次のボーンに引き渡す
			if (XMVector3Length(XMVectorSubtract(vecToEnd, vecToTarget)).m128_f32[0] <= epsilon)
			{
				continue;
			}

			// 外積計算および角度計算
			auto cross = XMVector3Normalize(XMVector3Cross(vecToEnd, vecToTarget)); // 軸になる

			// 便利な関数だが中身はcos（内積値）なので
			// 0～90°と0～-90°のの区別がない
			float angle = XMVector3AngleBetweenVectors(vecToEnd, vecToTarget).m128_f32[0];

			// 回転の限界を超えてしまったときは限界値に補正
			angle = min(angle, ikLimit);
			XMMATRIX rot = XMMatrixRotationAxis(cross, angle); // 回転行列作成

			// 原点中心ではなく、pos中心に回転
			auto mat = XMMatrixTranslationFromVector(-pos)
				* rot
				* XMMatrixTranslationFromVector(pos);

			// 回転行列を保持しておく（乗算で回転重ね掛けを作っておく）
			mats[bidx] *= mat;

			// 対象となる点をすべて回転させる（現在の点から見て末端側を回転）
			// なお、自分を回転させる必要はない
			for (auto idx = bidx - 1; idx >= 0; --idx)
			{
				bonePositions[idx] = XMVector3Transform(bonePositions[idx], mat);
			}

			endPos = XMVector3Transform(endPos, mat);

			// もし正解に近くなっていたらループを抜ける
			if (XMVector3Length(XMVectorSubtract(endPos, targetNextPos)).m128_f32[0] <= epsilon)
			{
				break;
			}
		}
	}
	int idx = 0;
	for (auto& cidx : ik.nodeIdxes)
	{
		m_boneMatrices[cidx] = mats[idx];
		++idx;
	}
	auto rootNode = m_boneNodeAddressArray[ik.nodeIdxes.back()];
	RecursiveMatrixMultiply(rootNode, parentMat);
}

void PMDActor::SolveCosineIK(const PMDIK & ik)
{
	// IK構成点を保存
	std::vector<XMVECTOR> positions;

	// IKのそれぞれのボーン間の距離を保存
	std::array<float, 2> edgeLens;

	// ターゲット（末端ボーンではなく、末端ボーンが近づく
	//             目標ボーンの座標を取得）
	auto& targetNode = m_boneNodeAddressArray[ik.boneIdx];
	auto targetPos = XMVector3Transform(
		XMLoadFloat3(&targetNode->startPos),
		m_boneMatrices[ik.boneIdx]);

	// IKチェーンが逆順なので、逆に並ぶようにしている

	// 末端ボーン
	auto endNode = m_boneNodeAddressArray[ik.targetIdx];
	positions.emplace_back(XMLoadFloat3(&endNode->startPos));

	// 中間およびルートボーン
	for (auto& chainBoneIdx : ik.nodeIdxes)
	{
		auto boneNode = m_boneNodeAddressArray[chainBoneIdx];
		positions.emplace_back(XMLoadFloat3(&boneNode->startPos));
	}

	// わかりづらいので逆にする
	// 問題ない人はそのまま計算してかまわない
	std::reverse(positions.begin(), positions.end());

	// 元の長さを測っておく
	edgeLens[0] = XMVector3Length(
		XMVectorSubtract(positions[1], positions[0])).m128_f32[0];

	edgeLens[1] = XMVector3Length(
		XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	// ルートボーン座標変換（逆順になっているため使用するインデックスに注意
	positions[0] = XMVector3Transform(positions[0], m_boneMatrices[ik.nodeIdxes[1]]);

	// 真ん中は自動計算されるので計算しない

	// 先端ボーン
	positions[2] = XMVector3Transform(positions[2], m_boneMatrices[ik.boneIdx]);

	// ルートから先頭へのベクトルを作っておく
	auto linearVec = XMVectorSubtract(positions[2], positions[0]);

	float a = XMVector3Length(linearVec).m128_f32[0];
	float b = edgeLens[0];
	float c = edgeLens[1];

	linearVec = XMVector3Normalize(linearVec);

	// ルートから真ん中への角度計算
	float theta1 = acosf((a * a + b * b - c * c) / (2 * a * b));

	// 真ん中からターゲットへの角度計算
	float theta2 = acosf((b * b + c * c - a * a) / (2 * b * c));

	// 「軸を求める」
	// もし真ん中が「ひざ」であった場合には強制的にX軸をする
	XMVECTOR axis;
	if (find(m_kneeIdxes.begin(), m_kneeIdxes.end(), ik.nodeIdxes[0]) == m_kneeIdxes.end())
	{
		auto vm = XMVector3Normalize(
			XMVectorSubtract(positions[2], positions[0]));

		auto vt = XMVector3Normalize(XMVectorSubtract(targetPos, positions[0]));

		axis = XMVector3Cross(vt, vm);
	}
	else
	{
		auto right = XMFLOAT3(1, 0, 0);
		axis = XMLoadFloat3(&right);
	}

	// 注意点：IKチェーンはルートに向かってから数えられるため 1 がルートに近い
	auto mat1 = XMMatrixTranslationFromVector(-positions[0]);
	mat1 *= XMMatrixRotationAxis(axis, theta1);
	mat1 *= XMMatrixTranslationFromVector(positions[0]);

	auto mat2 = XMMatrixTranslationFromVector(-positions[1]);
	mat2 *= XMMatrixRotationAxis(axis, theta2 - XM_PI);
	mat2 *= XMMatrixTranslationFromVector(positions[1]);

	m_boneMatrices[ik.nodeIdxes[1]] *= mat1;
	m_boneMatrices[ik.nodeIdxes[0]] = mat2 * m_boneMatrices[ik.nodeIdxes[1]];
	m_boneMatrices[ik.targetIdx] = m_boneMatrices[ik.nodeIdxes[0]];
}

void PMDActor::SolveLookAt(const PMDIK & ik)
{
	// この関数に来た時点でノードは1つしかなく、
	// チェーンに入っているのオード番号はIKのルートノードのものなので、
	// このルートノードから末端に向かうベクトルを考える
	auto rootNode = m_boneNodeAddressArray[ik.nodeIdxes[0]];
	auto targetNode = m_boneNodeAddressArray[ik.targetIdx];

	auto rpos1 = XMLoadFloat3(&rootNode->startPos);
	auto tpos1 = XMLoadFloat3(&targetNode->startPos);

	auto rpos2 = XMVector3Transform(rpos1, m_boneMatrices[ik.nodeIdxes[0]]);
	auto tpos2 = XMVector3Transform(tpos1, m_boneMatrices[ik.boneIdx]);

	auto originVec = XMVectorSubtract(tpos1, rpos1);
	auto targetVec = XMVectorSubtract(tpos2, rpos2);

	originVec = XMVector3Normalize(originVec);
	targetVec = XMVector3Normalize(targetVec);

	XMFLOAT3 up{ 0, 1, 0 };
	XMFLOAT3 right{ 1, 0, 0 };

	//m_boneMatrices[ik.nodeIdxes[0]]
	//	= LookAtMatrix(originVec, targetVec, up, right);

	XMMATRIX mat = XMMatrixTranslationFromVector(-rpos2)
		* LookAtMatrix(originVec, targetVec, up, right)
		* XMMatrixTranslationFromVector(rpos2);

	m_boneMatrices[ik.nodeIdxes[0]] = mat;
}

void PMDActor::IKSolve(int frameNo)
{
	// 前にも行ったように、IKオン/オフ情報をフレーム番号で逆から検索
	auto it = find_if(m_ikEnableData.rbegin(), m_ikEnableData.rend(),
		[frameNo](const VMDIKEnable& ikEnable)
	{
		return ikEnable.frameNo <= frameNo;
	});

	for (auto& ik : m_pmdIkData)
	{
		if (it != m_ikEnableData.rend())
		{
			auto ikEnableIt = it->ikEnableTable.find(m_boneNameArray[ik.boneIdx]);

			if (ikEnableIt != it->ikEnableTable.end())
			{
				if (!ikEnableIt->second) // もしオフなら打ち切る
				{
					continue;
				}
			}
		}

		auto childrenNodesCount = ik.nodeIdxes.size();

		switch (childrenNodesCount)
		{
		case 0: // 間のボーン数が 0（ありえない）
			assert(0);
			continue;

		case 1: // 間のボーン数が 1 のときはLookAt
			SolveLookAt(ik);
			break;

		case 2: // 間のボーン数が 2 のときは余弦定理IK
			SolveCosineIK(ik);
			break;

		default: // 3 以上のときは CCD_IK
			SolveCCDIK(ik);
		}
	}
}

PMDActor::PMDActor(const char * filePath, std::shared_ptr<Dx12Wrapper> dx12)
	: m_dx12(dx12)
	, m_angle(0.0f)
{
	m_transform.world = XMMatrixIdentity();
	LoadPMDFile(filePath);
	CreateTransformView();
	CreateMaterialData();
	CreateMaterialAndTextureView();

	//LoadVMDFile("motion/squat.vmd", "motion");
	PlayAnimation();
}

PMDActor::~PMDActor()
{
}

HRESULT PMDActor::LoadVMDFile(const char * path)
{
	auto fp = fopen(path, "rb");
	fseek(fp, 50, SEEK_SET); // 最初のバイトは飛ばしてOK

	unsigned int motionDataNum = 0;
	fread(&motionDataNum, sizeof(motionDataNum), 1, fp);

	struct VMDMotion
	{
		char boneName[15]; // ボーン名
		unsigned int frameNo; // フレーム番号
		XMFLOAT3 location; // 位置
		XMFLOAT4 quaternion; // クォータニオン
		unsigned char bezier[64]; // [4][4][4]ベジェ補間パラメーター
	};

	std::vector<VMDMotion> vmdMotionData(motionDataNum);

	for (auto& motion : vmdMotionData)
	{
		fread(motion.boneName, sizeof(motion.boneName), 1, fp); // ボーン名
		fread(&motion.frameNo,
			sizeof(motion.frameNo)      // フレーム番号
			+ sizeof(motion.location)   // 位置（IKのときに使用予定）
			+ sizeof(motion.quaternion) // クォータニオン
			+ sizeof(motion.bezier),    // 補間ベジェデータ
			1, fp);
	}

#pragma pack(1)

	// 表情データ
	struct VMDMorph
	{
		char name[15]; // 名前（パディングしてしまう）
		uint32_t frameNo; // フレーム番号
		float weight; // ウェイト（0.0f～1.0f）
	}; // 全部で23バイトなので#pragma packで読む

#pragma pack()

	uint32_t morphCount = 0;
	fread(&morphCount, sizeof(morphCount), 1, fp);

	std::vector<VMDMorph> morphs(morphCount);
	fread(morphs.data(), sizeof(VMDMorph), morphCount, fp);

#pragma pack(1)

	// カメラ
	struct VMDCamera
	{
		uint32_t framNo; // フレーム番号
		float distance; // 距離
		XMFLOAT3 pos; // 座標
		XMFLOAT3 eulerAngle; // オイラー角
		uint8_t interpolation[24]; // 補間
		uint32_t fov; // 視野角
		uint8_t persFlg; // パースフラグ ON/OFF
	}; // 61バイト（これも#pragma packの必要あり）

#pragma pack()

	uint32_t vmdCameraCount = 0;
	fread(&vmdCameraCount, sizeof(vmdCameraCount), 1, fp);

	std::vector<VMDCamera> cameraData(vmdCameraCount);
	fread(cameraData.data(), sizeof(VMDCamera), vmdCameraCount, fp);

	// ライト証明データ
	struct VMDLight
	{
		uint32_t frameNo; // フレーム番号
		XMFLOAT3 rgb; // ライト色
		XMFLOAT3 vec; // 光線ベクトル（平行光線）
	};

	uint32_t vmdLightCount = 0;
	fread(&vmdLightCount, sizeof(vmdLightCount), 1, fp);

	std::vector<VMDLight> lights(vmdLightCount);
	fread(lights.data(), sizeof(VMDLight), vmdLightCount, fp);

#pragma pack(1)

	// セルフ影データ
	struct VMDSelfShadow
	{
		uint32_t frameNo; // フレーム番号
		uint8_t mode; // 影モード（0:影なし、1:モード1、2:モード2）
		float distance; // 距離
	};

#pragma pack()

	uint32_t selfShadowCount = 0;
	fread(&selfShadowCount, sizeof(selfShadowCount), 1, fp);

	std::vector<VMDSelfShadow> selfShadowData(selfShadowCount);
	fread(selfShadowData.data(), sizeof(VMDSelfShadow), selfShadowCount, fp);

	// IKオン/オフの切り替わり数
	uint32_t ikSwitchCount = 0;
	fread(&ikSwitchCount, sizeof(ikSwitchCount), 1, fp);

	m_ikEnableData.resize(ikSwitchCount);

	for (auto& ikEnable : m_ikEnableData)
	{
		// キーフレーム情報なのでまずはフレーム番号読み込み
		fread(&ikEnable.frameNo, sizeof(ikEnable.frameNo), 1, fp);

		// 次に菓子フラグがあるが、これは使用しないため
		// 1バイトシークでもかまわない
		uint8_t visibleFlg = 0;
		fread(&visibleFlg, sizeof(visibleFlg), 1, fp);

		// 対象ボーン数読み込み
		uint32_t ikBoneCount = 0;
		fread(&ikBoneCount, sizeof(ikBoneCount), 1, fp);

		// ループしつつ名前とON/OFF情報を取得
		for (int i = 0; i < ikBoneCount; ++i)
		{
			char ikBoneName[20];
			fread(ikBoneName, _countof(ikBoneName), 1, fp);

			uint8_t flg = 0;
			fread(&flg, sizeof(flg), 1, fp);
			ikEnable.ikEnableTable[ikBoneName] = flg;
		}
	}

	fclose(fp);

	// VMDのモーションデータから、実際に使用するモーションテーブルへ変換
	for (auto& vmdMotion : vmdMotionData)
	{
		m_motionData[vmdMotion.boneName].emplace_back(
			Motion(vmdMotion.frameNo, XMLoadFloat4(&vmdMotion.quaternion), vmdMotion.location,
				XMFLOAT2((float)vmdMotion.bezier[3] / 127.0f, (float)vmdMotion.bezier[7] / 127.0f),
				XMFLOAT2((float)vmdMotion.bezier[11] / 127.0f, (float)vmdMotion.bezier[15] / 127.0f)));

		m_duration = std::max<unsigned int>(m_duration, vmdMotion.frameNo);
	}

	for (auto& motion : m_motionData)
	{
		std::sort(motion.second.begin(), motion.second.end(),
			[](const Motion& lval, const Motion& rval)
		{
			return lval.frameNo <= rval.frameNo;
		});
	}

	for (auto& boneMotion : m_motionData)
	{
		auto itBoneNode = m_boneNodeTable.find(boneMotion.first);

		if (itBoneNode == m_boneNodeTable.end())
		{
			continue;
		}

		auto& node = itBoneNode->second;
		auto& pos = node.startPos;
		auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* XMMatrixRotationQuaternion(boneMotion.second[0].quaternion)
			* XMMatrixTranslation(pos.x, pos.y, pos.z);

		m_boneMatrices[node.boneIdx] = mat;
	}



	RecursiveMatrixMultiply(&m_boneNodeTable["センター"], XMMatrixIdentity());

	copy(m_boneMatrices.begin(), m_boneMatrices.end(), m_mappedMatrices + 1);

	return S_OK;
}

void PMDActor::Update()
{
	//m_angle += 0.001f;
	//m_mappedMatrices[0] = XMMatrixRotationY(m_angle);
	m_mappedMatrices[0] = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);

	MotionUpdate();
}

void PMDActor::Draw(bool isShadow)
{
	m_dx12->CommandList()->IASetVertexBuffers(0, 1, &m_vbView);
	m_dx12->CommandList()->IASetIndexBuffer(&m_ibView);
	m_dx12->CommandList()->IASetPrimitiveTopology(
		D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* transHeaps[] = { m_transformHeap.Get() };
	m_dx12->CommandList()->SetDescriptorHeaps(1, transHeaps);
	m_dx12->CommandList()->SetGraphicsRootDescriptorTable(1,
		m_transformHeap->GetGPUDescriptorHandleForHeapStart());

	ID3D12DescriptorHeap* mdh[] = { m_materialDescHeap.Get() };
	m_dx12->CommandList()->SetDescriptorHeaps(1, mdh);

	auto materialH = m_materialDescHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0; // 最初はオフセットなし

	auto cbvsrvIncSize =
		m_dx12->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;

	if (isShadow)
	{
		m_dx12->CommandList()->DrawIndexedInstanced(m_indicesNum, 1, 0, 0, 0);
	}
	else
	{
		for (auto& m : m_materials)
		{
			m_dx12->CommandList()->SetGraphicsRootDescriptorTable(2, materialH);

			m_dx12->CommandList()->DrawIndexedInstanced(m.indicesNum, 2, idxOffset, 0, 0);

			// ヒープポインターとインデックスを次に進める
			materialH.ptr += cbvsrvIncSize;
			idxOffset += m.indicesNum;
		}
	}
}

void PMDActor::PlayAnimation()
{
	m_startTime = timeGetTime();
}

void PMDActor::SetPos(float x, float y, float z)
{
	m_pos.x = x;
	m_pos.y = y;
	m_pos.z = z;
}

//void * PMDActor::Transform::operator new(size_t size)
//{
//	return _aligned_malloc(size, 16);
//}
