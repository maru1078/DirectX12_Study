#include "PMDActor.h"

#include <algorithm>
#include <sstream>
#include <array>
#include "../Dx12Wrapper/Dx12Wrapper.h"
#include "../Helper/Helper.h"

#pragma comment(lib, "winmm.lib") // timeGetTime()を呼ぶのに必要

// 誤差の範囲内かどうかに使用する定数
constexpr float epsilon = 0.0005f;

// z軸方向を特定の方向に向ける行列を返す関数
// @param lookat 向かせたい方向ベクトル
// @param up 上ベクトル
// @param right 右ベクトル
XMMATRIX LookAtMatrix(const XMVECTOR& lookat, const XMFLOAT3& up, const XMFLOAT3& right)
{
	// 向かせたい方向（z軸）
	XMVECTOR vz = lookat;

	// （向かせたい方向を向かせた時の）仮のy軸ベクトル
	XMVECTOR vy = XMVector3Normalize(XMLoadFloat3(&up));

	// （向かせたい方向を向かせた時の）y軸
	XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));
	vy = XMVector3Normalize(XMVector3Cross(vz, vx));

	// LookAtとupが同じ方向を向いていたらrightを基準にして作り直す
	if (std::abs(XMVector3Dot(vy, vz).m128_f32[0]) == 1.0f)
	{
		// 仮のx方向を定義
		vx = XMVector3Normalize(XMLoadFloat3(&right));

		// 向かせたい方向を向かせた時のY軸を計算
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

// 特定のベクトルを特定の方向に向ける行列を返す関数
// @param origin 特定のベクトル
// @param lookat 向かせたい方向ベクトル
// @param up 上ベクトル
// @param right 右ベクトル
// @return 特定のベクトルを特定の方向に向けるための行列
XMMATRIX LookAtMatrix(const XMVECTOR& origin, const XMVECTOR& lookat, const XMFLOAT3& up, const XMFLOAT3& right)
{
	// ★どういうことだ？
	return XMMatrixTranspose(LookAtMatrix(origin, up, right)
		* LookAtMatrix(lookat, up, right));
}

PMDActor::PMDActor(const std::string & filePath, std::weak_ptr<Dx12Wrapper> dx12, XMFLOAT3 pos)
	: m_dx12{ dx12 }
	, m_pos{ pos }
{
	if (!LoadPMDFile(filePath))
	{
		assert(0);
	}
	if (!LoadVMDFile("Motion/pose.vmd"))
	{
		assert(0);
	}
	if (!CreateMaterialBuffer())
	{
		assert(0);
	}
	if (!CreateMaterialBufferView())
	{
		assert(0);
	}
	if (!CreateVertexBuffer())
	{
		assert(0);
	}
	if (!CreateIndexBuffer())
	{
		assert(0);
	}
	if (!CreateTransformBuffer())
	{
		assert(0);
	}
	if (!CreateTransformBufferView())
	{
		assert(0);
	}

	PlayAnimation();
}

void PMDActor::PlayAnimation()
{
	m_startTime = timeGetTime();
}

void PMDActor::MotionUpdate()
{
	auto elaspedTime = timeGetTime() - m_startTime;
	unsigned int frameNo = 30 * (elaspedTime / 1000.0f);

	// ここからループのための追加コード
	if (frameNo > m_duration)
	{
		m_startTime = timeGetTime();
		frameNo = 0;
	}

	// 行列情報のクリア
	std::fill(m_boneMatrices.begin(), m_boneMatrices.end(), XMMatrixIdentity());

	// モーションデータ更新
	for (auto& boneMotion : m_motionData)
	{
		// モデルデータ側に存在しているボーンかどうか調べる
		auto itBoneNode = m_boneNodeTable.find(boneMotion.first);
		
		// 存在していなかった場合、処理を飛ばす
		if (itBoneNode == m_boneNodeTable.end())
		{
			continue;
		}

		auto node = itBoneNode->second;

		// 合致するものを探す
		auto motions = boneMotion.second;
		auto rit = std::find_if(motions.rbegin(), motions.rend(), 
			[frameNo](const Motion& motion)
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
			auto t = static_cast<float>(frameNo - rit->frameNo)
				/ static_cast<float>(it->frameNo - rit->frameNo);

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

	std::copy(m_boneMatrices.begin(), m_boneMatrices.end(), m_mappedMatrices + 1);
}

void PMDActor::Update()
{
	MotionUpdate();

	//m_angle += 0.01f;
	m_mappedMatrices[0] = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z)
		* XMMatrixRotationY(m_angle);
}

void PMDActor::Draw(bool isShadow)
{
	// プリミティブトポロジをセット
	m_dx12.lock()->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点バッファをセット
	m_dx12.lock()->CommandList()->IASetVertexBuffers(0, 1, &m_vbView);

	// インデックスバッファをセット
	m_dx12.lock()->CommandList()->IASetIndexBuffer(&m_ibView);

	// ワールド行列を入れるヒープを設定
	m_dx12.lock()->CommandList()->SetDescriptorHeaps(1, m_transformHeap.GetAddressOf());

	m_dx12.lock()->CommandList()->SetGraphicsRootDescriptorTable(
		1, m_transformHeap->GetGPUDescriptorHandleForHeapStart());

	// ディスクリプタヒープをセット（マテリアル用）
	m_dx12.lock()->CommandList()->SetDescriptorHeaps(1, m_materialDescHeap.GetAddressOf());

	auto materialH = m_materialDescHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	if (isShadow)
	{
		m_dx12.lock()->CommandList()->DrawIndexedInstanced(m_indexNum,
			1, 0, 0, 0);
	}
	else
	{
		for (auto& m : m_materials)
		{
			// ルートパラメーターとディスクリプタヒープの関連付け（マテリアル用）
			m_dx12.lock()->CommandList()->SetGraphicsRootDescriptorTable(
				2, materialH);

			// ドローコール
			m_dx12.lock()->CommandList()->DrawIndexedInstanced(m.indicesNum,
				2, idxOffset, 0, 0);

			// ヒープポインターとインデックスを先に進める
			materialH.ptr += m_dx12.lock()->Device()->
				GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
			idxOffset += m.indicesNum;
		}
	}
}

bool PMDActor::LoadPMDFile(const std::string & filePath)
{
	char signature[3]{}; // pmdファイルの先頭3バイトが「Pmd」となっているため、それは構造体に含めない
	FILE* fp;
	PMDHeader pmdHeader;
	unsigned int vertNum; // 頂点数
	unsigned int materialNum; // マテリアル数
	std::vector<PMDMaterial> pmdMaterials;
	unsigned short boneNum;
	std::vector<PMDBone> pmdBones;
	uint16_t ikNum;

	fopen_s(&fp, filePath.c_str(), "rb");
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	fread(&vertNum, sizeof(vertNum), 1, fp);
	m_vertices.resize(vertNum * sizeof(PMDVertex));
	fread(m_vertices.data(), m_vertices.size(), 1, fp);

	fread(&m_indexNum, sizeof(m_indexNum), 1, fp);
	m_indices.resize(m_indexNum);
	fread(m_indices.data(), m_indices.size() * sizeof(m_indices[0]), 1, fp);

	fread(&materialNum, sizeof(materialNum), 1, fp);
	pmdMaterials.resize(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);
	m_materials.resize(pmdMaterials.size());

	fread(&boneNum, sizeof(boneNum), 1, fp);
	pmdBones.resize(boneNum);
	fread(pmdBones.data(), sizeof(PMDBone), pmdBones.size(), fp);

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

	// コピー
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		m_materials[i].indicesNum = pmdMaterials[i].indicesNum;
		m_materials[i].material.diffuse = pmdMaterials[i].diffuse;
		m_materials[i].material.alpha = pmdMaterials[i].alpha;
		m_materials[i].material.specular = pmdMaterials[i].specular;
		m_materials[i].material.specularity = pmdMaterials[i].specularity;
		m_materials[i].material.ambient = pmdMaterials[i].ambient;
	}

	m_texResources.resize(materialNum);
	m_sphResources.resize(materialNum);
	m_spaResources.resize(materialNum);
	m_toonResources.resize(materialNum);

	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		// トゥーンリソースの読み込み
		std::string toonFilePath = "Toon/";
		char toonFileName[16];
		sprintf_s(
			toonFileName,
			"toon%02d.bmp",
			pmdMaterials[i].toonIdx + 1);

		toonFilePath += toonFileName;
		m_toonResources[i] = m_dx12.lock()->GetTextureByPath(toonFilePath);

		if (strlen(pmdMaterials[i].texFilePath) == 0)
		{
			continue;
		}

		std::string fileName = pmdMaterials[i].texFilePath;
		std::string texFileName = "";
		std::string sphFileName = "";
		std::string spaFileName = "";

		if (std::count(fileName.begin(), fileName.end(), '*') > 0)
		{ // スプリッタがある
			auto namePair = SplitFileName(fileName);

			if (GetExtension(namePair.first) == "sph")
			{
				texFileName = namePair.second;
				sphFileName = namePair.first;
			}
			else if (GetExtension(namePair.first) == "spa")
			{
				texFileName = namePair.second;
				spaFileName = namePair.first;
			}
			else
			{
				texFileName = namePair.first;

				if (GetExtension(namePair.second) == "sph")
				{
					sphFileName = namePair.second;
				}
				else if (GetExtension(namePair.second) == "spa")
				{
					spaFileName = namePair.second;
				}
			}
		}
		else
		{
			if (GetExtension(pmdMaterials[i].texFilePath) == "sph")
			{
				sphFileName = pmdMaterials[i].texFilePath;
			}
			else if (GetExtension(pmdMaterials[i].texFilePath) == "spa")
			{
				spaFileName = pmdMaterials[i].texFilePath;;
			}
			else
			{
				texFileName = pmdMaterials[i].texFilePath;
			}
		}

		// モデルとテクスチャパスからアプリケーションからのテクスチャパスを得る
		if (texFileName != "")
		{
			auto texFilePath = GetTexturePathFromModelAndTexPath(
				filePath,
				texFileName.c_str());

			m_texResources[i] = m_dx12.lock()->GetTextureByPath(texFilePath);
		}
		if (sphFileName != "")
		{
			auto sphFilePath = GetTexturePathFromModelAndTexPath(
				filePath,
				sphFileName.c_str());

			m_sphResources[i] = m_dx12.lock()->GetTextureByPath(sphFilePath);
		}
		if (spaFileName != "")
		{
			auto spaFilePath = GetTexturePathFromModelAndTexPath(
				filePath,
				spaFileName.c_str());

			m_spaResources[i] = m_dx12.lock()->GetTextureByPath(spaFilePath);
		}
	}

	// +--------------------------------------+
	// |     ボーン情報をノードテーブルへ     |
	// +--------------------------------------+

	// インデックスと名前の対応関係構築のためにあとで使う
	std::vector<std::string> boneNames{ pmdBones.size() };
	m_boneNameArray.resize(pmdBones.size());
	m_boneNodeAddressArray.resize(pmdBones.size());
	m_kneeIdxes.clear();

	// ボーンノードマップを作る
	for (int i = 0; i < pmdBones.size(); ++i)
	{
		auto& pb = pmdBones[i];
		boneNames[i] = pb.boneName;

		auto& node = m_boneNodeTable[pb.boneName];
		node.boneIdx = i;
		node.startPos = pb.pos;

		// インデックス検索がしやすいように
		m_boneNameArray[i] = pb.boneName;
		m_boneNodeAddressArray[i] = &node;

		// ひざを取っておく
		std::string boneName = pb.boneName;

		if (boneName.find("ひざ") != std::string::npos)
		{
			m_kneeIdxes.emplace_back(i);
		}
	}

	// 親子関係を構築する
	for (auto& pb : pmdBones)
	{
		// 親インデックスをチェック（ありえない数字なら飛ばす）
		if (pb.parentNo >= pmdBones.size())
		{
			continue;
		}

		// 親番号から親のボーン名を取得
		auto parentName = boneNames[pb.parentNo];

		// 親の子ノード情報に追加
		m_boneNodeTable[parentName].children.emplace_back(
			&m_boneNodeTable[pb.boneName]);
	}

	m_boneMatrices.resize(pmdBones.size());

	// ボーンをすべて初期化する
	std::fill(m_boneMatrices.begin(), m_boneMatrices.end(), XMMatrixIdentity());

	return true;
}

bool PMDActor::LoadVMDFile(const std::string & filePath)
{
	FILE* fp;
	unsigned int motionDataNum;
	std::vector<VMDMotion> vmdMotionData;
	uint32_t morphCount = 0;
	std::vector<VMDMorph> morphs;
	uint32_t vmdCameraCount = 0;
	std::vector<VMDCamera> cameraData;
	uint32_t vmdLightCount = 0;
	std::vector<VMDLight> lights;
	uint32_t selfShadowCount = 0;
	std::vector<VMDSelfShadow> selfShadowData;
	uint32_t ikSwitchCount = 0; // IKオン/オフの切り替わり

	fopen_s(&fp, filePath.c_str(), "rb");
	fseek(fp, 50, SEEK_SET); // 最初の50バイトは飛ばす

	fread(&motionDataNum, sizeof(motionDataNum), 1, fp);
	vmdMotionData.resize(motionDataNum);

	for (auto& motion : vmdMotionData)
	{
		fread(motion.boneName, sizeof(motion.boneName), 1, fp); // ボーン名
		fread(&motion.frameNo,
			sizeof(motion.frameNo)		// フレーム番号
			+ sizeof(motion.location)	// 位置
			+ sizeof(motion.quaternion) // クォータニオン
			+ sizeof(motion.bezier),	// 補間ベジェデータ
			1, fp);

		m_duration = std::max<unsigned int>(m_duration, motion.frameNo);
	}

	fread(&morphCount, sizeof(morphCount), 1, fp);
	morphs.resize(morphCount);
	fread(morphs.data(), sizeof(VMDMorph), morphs.size(), fp);

	fread(&vmdCameraCount, sizeof(vmdCameraCount), 1, fp);
	cameraData.resize(vmdCameraCount);
	fread(cameraData.data(), sizeof(VMDCamera), cameraData.size(), fp);

	fread(&vmdLightCount, sizeof(vmdLightCount), 1, fp);
	lights.resize(vmdLightCount);
	fread(lights.data(), sizeof(VMDCamera), lights.size(), fp);

	fread(&selfShadowCount, sizeof(selfShadowCount), 1, fp);
	selfShadowData.resize(selfShadowCount);
	fread(selfShadowData.data(), sizeof(VMDSelfShadow), selfShadowData.size(), fp);

	fread(&ikSwitchCount, sizeof(ikSwitchCount), 1, fp);
	m_ikEnableData.resize(ikSwitchCount);

	for (auto& ikEnable : m_ikEnableData)
	{
		// キーフレーム情報なのでまずはフレーム番号読み込み
		fread(&ikEnable.frameNo, sizeof(ikEnable.frameNo), 1, fp);

		// 次に可視フラグがあるが、これは使用しないため
		// 1バイトシークでも構わない
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

	// VMDモーションデータから、実際に使用するモーションテーブルへ変換
	for (auto& vmdMotion : vmdMotionData)
	{
		m_motionData[vmdMotion.boneName].emplace_back(
			Motion(vmdMotion.frameNo,XMLoadFloat4(&vmdMotion.quaternion), vmdMotion.location,
				XMFLOAT2((float)vmdMotion.bezier[3] / 127.0f, (float)vmdMotion.bezier[7] / 127.0f),
				XMFLOAT2((float)vmdMotion.bezier[11] / 127.0f, (float)vmdMotion.bezier[15] / 127.0f)));
	}

	// フレーム番号によるソート
	for (auto& motion : m_motionData)
	{
		std::sort(motion.second.begin(), motion.second.end(),
			[](const Motion& lval, const Motion& rval)
		{
			return lval.frameNo <= rval.frameNo;
		});
	}

	return true;
}

bool PMDActor::CreateMaterialBuffer()
{
	// マテリアルバッファー作成
	auto materialBuffSize = (sizeof(MaterialForHlsl) + 0xff) & ~0xff;

	auto result = m_dx12.lock()->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * m_materials.size()), // もったいないやり方
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_materialBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// マップマテリアルにコピー
	char* mapMaterial{ nullptr };
	result = m_materialBuff->Map(0, nullptr, (void**)&mapMaterial);

	if (FAILED(result)) return false;

	for (auto& m : m_materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material; // データコピー
		mapMaterial += materialBuffSize; // 次のアラインメント位置まで進める（256の倍数）
	}

	m_materialBuff->Unmap(0, nullptr);

	return true;
}

bool PMDActor::CreateMaterialBufferView()
{
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc{};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = m_materials.size() * 5; // マテリアル数 * 5を指定
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = m_dx12.lock()->Device()->CreateDescriptorHeap(&matDescHeapDesc,
		IID_PPV_ARGS(m_materialDescHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// マテリアル用定数バッファービュー作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc{};
	matCBVDesc.BufferLocation = m_materialBuff->GetGPUVirtualAddress(); // バッファアドレス
	matCBVDesc.SizeInBytes = AlignmentedSize(sizeof(MaterialForHlsl),
		D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT); // マテリアルの256アラインメントサイズ

	// マテリアル用シェーダーリソースビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // デフォルト
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1; // ミップマップは使用しないので 1

	auto matDescHeapH = m_materialDescHeap->GetCPUDescriptorHandleForHeapStart(); // 先頭を記録
	auto inc = m_dx12.lock()->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < m_materials.size(); ++i)
	{
		// 定数バッファービュー
		m_dx12.lock()->Device()->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += inc;
		matCBVDesc.BufferLocation += AlignmentedSize(sizeof(MaterialForHlsl),
			D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		// シェーダーリソースビュー
		if (m_texResources[i] != nullptr)
		{
			srvDesc.Format = m_texResources[i]->GetDesc().Format;
			m_dx12.lock()->Device()->CreateShaderResourceView(
				m_texResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}
		else
		{
			srvDesc.Format = m_dx12.lock()->WhiteTex()->GetDesc().Format;
			m_dx12.lock()->Device()->CreateShaderResourceView(
				m_dx12.lock()->WhiteTex().Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += inc;

		if (m_sphResources[i] != nullptr)
		{
			srvDesc.Format = m_sphResources[i]->GetDesc().Format;
			m_dx12.lock()->Device()->CreateShaderResourceView(
				m_sphResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}
		else
		{
			srvDesc.Format = m_dx12.lock()->WhiteTex()->GetDesc().Format;
			m_dx12.lock()->Device()->CreateShaderResourceView(
				m_dx12.lock()->WhiteTex().Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += inc;

		if (m_spaResources[i] != nullptr)
		{
			srvDesc.Format = m_spaResources[i]->GetDesc().Format;
			m_dx12.lock()->Device()->CreateShaderResourceView(
				m_spaResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}
		else
		{
			srvDesc.Format = m_dx12.lock()->BlackTex()->GetDesc().Format;
			m_dx12.lock()->Device()->CreateShaderResourceView(
				m_dx12.lock()->BlackTex().Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += inc;

		if (m_toonResources[i] != nullptr)
		{
			srvDesc.Format = m_toonResources[i]->GetDesc().Format;
			m_dx12.lock()->Device()->CreateShaderResourceView(
				m_toonResources[i].Get(),
				&srvDesc,
				matDescHeapH);
		}
		else
		{
			srvDesc.Format = m_dx12.lock()->GrayGradTex()->GetDesc().Format;
			m_dx12.lock()->Device()->CreateShaderResourceView(
				m_dx12.lock()->GrayGradTex().Get(),
				&srvDesc,
				matDescHeapH);
		}

		matDescHeapH.ptr += inc;
	}

	return true;
}

bool PMDActor::CreateVertexBuffer()
{
	auto result = m_dx12.lock()->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(m_vertices.size()), // サイズ変更
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_vertBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	unsigned char* vertMap{ nullptr };
	result = m_vertBuff->Map(0, nullptr, (void**)&vertMap);

	if (FAILED(result)) return false;

	std::copy(m_vertices.begin(), m_vertices.end(), vertMap);
	m_vertBuff->Unmap(0, nullptr);

	m_vbView.BufferLocation = m_vertBuff->GetGPUVirtualAddress(); // バッファの仮想アドレス
	m_vbView.SizeInBytes = m_vertices.size(); // 全バイト数
	m_vbView.StrideInBytes = sizeof(PMDVertex); // 1頂点あたりのバイト数

	return true;
}

bool PMDActor::CreateIndexBuffer()
{
	auto result = m_dx12.lock()->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(m_indices.size() * sizeof(m_indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_idxBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	m_ibView.BufferLocation = m_idxBuff->GetGPUVirtualAddress();
	m_ibView.Format = DXGI_FORMAT_R16_UINT;
	m_ibView.SizeInBytes = m_indices.size() * sizeof(m_indices[0]);

	// 作ったバッファーにインデックスデータをコピー
	unsigned short* mappedIdx{ nullptr };
	result = m_idxBuff->Map(0, nullptr, (void**)&mappedIdx);

	if (FAILED(result)) return false;

	std::copy(m_indices.begin(), m_indices.end(), mappedIdx);
	m_idxBuff->Unmap(0, nullptr);

	return true;
}

bool PMDActor::CreateTransformBuffer()
{
	// ヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // CBV
	auto result = m_dx12.lock()->Device()->CreateDescriptorHeap(
		&descHeapDesc, IID_PPV_ARGS(m_transformHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	auto buffSize = sizeof(XMMATRIX) * (1 + m_boneMatrices.size());
	buffSize = (buffSize + 0xff) & ~0xff;

	// 定数バッファ作成
	result = m_dx12.lock()->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(buffSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_transformBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// マップ
	result = m_transformBuffer->Map(0, nullptr, (void**)&m_mappedMatrices);

	if (FAILED(result)) return false;

	m_mappedMatrices[0] = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);
	std::copy(m_boneMatrices.begin(), m_boneMatrices.end(), m_mappedMatrices + 1);

	m_transformBuffer->Unmap(0, nullptr);

	return true;
}

bool PMDActor::CreateTransformBufferView()
{
	auto buffSize = sizeof(XMMATRIX) * (1 + m_boneMatrices.size());
	buffSize = (buffSize + 0xff) & ~0xff;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = m_transformBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = buffSize;

	// ビュー作成
	m_dx12.lock()->Device()->CreateConstantBufferView(
		&cbvDesc,
		m_transformHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void PMDActor::RecursiveMatrixMultiply(BoneNode * node, const XMMATRIX & mat)
{
	m_boneMatrices[node->boneIdx] *= mat;

	for (auto& cnode : node->children)
	{
		RecursiveMatrixMultiply(cnode, m_boneMatrices[node->boneIdx]);
	}
}

float PMDActor::GetYFromXOnBezier(float x, const XMFLOAT2 & a, const XMFLOAT2 & b, uint8_t n)
{
	if (a.x == a.y && b.x == b.y)
	{
		return x; // 計算不要
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x; // t^3の係数
	const float k1 = 3 * b.x - 6 * a.x;     // t^2の係数
	const float k2 = 3 * a.x;               // t  の係数

	// tを近似で求める
	for (int i = 0; i < n; ++i)
	{
		// f(t)を求める
		auto ft = (k0 * t * t * t) + (k1 * t * t) + (k2 * t) - x;

		// もし結果が0に近い（誤差の範囲内）なら打ち切る
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}

		t -= ft / 2; // 刻む
	}

	// 求めたいtはすでに求めているのでyを計算する
	auto r = 1 - t;

	return (t * t * t) + (3 * t * t * r * b.y) + (3 * t * r * r * a.y);
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

	// 回転を記録
	std::vector<XMMATRIX> mats(bonePositions.size());
	std::fill(mats.begin(), mats.end(), XMMatrixIdentity());

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

		// bonePositionsは、CCD-IKにおける各ノードの座標を
		// ベクタ配列にしたもの。
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
			// 外積できないため次のボーンを引き渡す
			if (XMVector3Length(XMVectorSubtract(vecToEnd, vecToTarget)).m128_f32[0] <= epsilon)
			{
				continue;
			}

			// 外積計算および角度計算
			auto cross = XMVector3Normalize(XMVector3Cross(vecToEnd, vecToTarget)); // 軸になる

			// 便利な関数だが中身はcos（内積値）なので
			// 0 ～ 90°と0 ～ -90°の区別がない
			float angle = XMVector3AngleBetweenVectors(vecToEnd, vecToTarget).m128_f32[0];

			// 回転限界を超えてしまったときは限界値に補正
			angle = min(angle, ikLimit);
			XMMATRIX rot = XMMatrixRotationAxis(cross, angle); // 回転行列作成

			// 原点中心ではなくpos中心に回転
			auto mat = XMMatrixTranslationFromVector(-pos)
				* rot
				* XMMatrixTranslationFromVector(pos);

			// 回転行列を保持しておく
			mats[bidx] *= mat;

			// 対象となる点をすべて回転させる（現在の点から見て末端側を回転）
			// なお自分を回転させる必要はない
			for (auto idx = bidx - 1; idx >= 0; --idx)
			{
				bonePositions[idx] = XMVector3Transform(bonePositions[idx], mat);
			}

			endPos = XMVector3Transform(endPos, mat);

			// もし正解に近くなったらループを抜ける
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

	// ターゲット（末端ボーンではなく、末端ボーンが近づく目標ボーンの座標を取得）
	auto& targetNode = m_boneNodeAddressArray[ik.boneIdx];
	auto targetPos = XMVector3Transform(XMLoadFloat3(&targetNode->startPos), m_boneMatrices[ik.boneIdx]);

	// IKチェーンが逆順なので、逆に並ぶようにしている

	// 末端ボーン
	auto endNode = m_boneNodeAddressArray[ik.targetIdx];
	positions.emplace_back(XMLoadFloat3(&endNode->startPos));

	// 中間及びルートボーン
	for (auto& chainBoneIdx : ik.nodeIdxes)
	{
		auto boneNode = m_boneNodeAddressArray[chainBoneIdx];
		positions.emplace_back(XMLoadFloat3(&boneNode->startPos));
	}

	// わかりづらいので逆にする
	std::reverse(positions.begin(), positions.end());

	// 元の長さを測っておく
	edgeLens[0] = XMVector3Length(XMVectorSubtract(positions[1], positions[0])).m128_f32[0];
	edgeLens[1] = XMVector3Length(XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	// ルートボーン座標変換（逆順になっているため使用するインデックス注意）
	positions[0] = XMVector3Transform(positions[0], m_boneMatrices[ik.nodeIdxes[1]]);

	// 真ん中は自動計算されるので計算しない

	positions[2] = XMVector3Transform(positions[2], m_boneMatrices[ik.boneIdx]);

	// ルートから先端へのベクトルを作っておく
	auto linearVec = XMVectorSubtract(positions[2], positions[0]);

	float a = XMVector3Length(linearVec).m128_f32[0];
	float b = edgeLens[0];
	float c = edgeLens[1];

	linearVec = XMVector3Normalize(linearVec);

	// ルートから真ん中への角度計算
	float theta1 = acosf((a * a + b * b - c * c) / (2 * a * b));

	// 真ん中からターゲットへの角度計算
	float theta2 = acosf((b * b + c * c - a * a) / (2 * b * c));

	// 軸を求める
	// もし真ん中が「ひざ」であった場合には強制的にX軸とする
	XMVECTOR axis;

	if (std::find(m_kneeIdxes.begin(), m_kneeIdxes.end(), ik.nodeIdxes[0]) == m_kneeIdxes.end())
	{
		auto vm = XMVector3Normalize(XMVectorSubtract(positions[2], positions[0]));
		auto vt = XMVector3Normalize(XMVectorSubtract(targetPos, positions[0]));
		axis = XMVector3Cross(vt, vm);
	}
	else
	{
		auto right = XMFLOAT3(1, 0, 0);
		axis = XMLoadFloat3(&right);
	}

	// 注意点：IKチェーンはルートに向かってから数えられるため1がルートに近い
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
	/*
	この関数に来た時点でノードは1つしかなく、
	チェーンに入っているノード番号はIKのルートノードのものなので、
	このルートノードから末端に向かうベクトルを考える
	*/
	auto rootNode = m_boneNodeAddressArray[ik.nodeIdxes[0]];
	auto targetNode = m_boneNodeAddressArray[ik.boneIdx];

	auto rpos1 = XMLoadFloat3(&rootNode->startPos);
	auto tpos1 = XMLoadFloat3(&targetNode->startPos);

	auto rpos2 = XMVector3TransformCoord(rpos1, m_boneMatrices[ik.nodeIdxes[0]]);
	auto tpos2 = XMVector3TransformCoord(tpos1, m_boneMatrices[ik.boneIdx]);

	auto originVec = XMVectorSubtract(tpos1, rpos1);
	auto targetVec = XMVectorSubtract(tpos2, rpos2);

	originVec = XMVector3Normalize(originVec);
	targetVec = XMVector3Normalize(targetVec);

	m_boneMatrices[ik.nodeIdxes[0]] = LookAtMatrix(originVec, targetVec, XMFLOAT3(0, 1, 0), XMFLOAT3(1, 0, 0));
}

void PMDActor::IKSolve(int frameNo)
{
	// 前にも行ったように、IKオン/オフ情報をフレーム番号で逆から検索
	auto it = std::find_if(m_ikEnableData.rbegin(), m_ikEnableData.rend(),
		[frameNo](const VMDIKEnable& ikenable)
	{
		return ikenable.frameNo <= frameNo;
	});

	for (auto& ik : m_pmdIkData)
	{
		if (it != m_ikEnableData.rend())
		{
			auto ikEnableIt = it->ikEnableTable.find(m_boneNameArray[ik.boneIdx]);

			if (ikEnableIt != it->ikEnableTable.end())
			{
				// もしIKがオフなら打ち切る
				if (!ikEnableIt->second)
				{
					continue;
				}
			}
		}

		auto childrenNodesCount = ik.nodeIdxes.size();

		switch (childrenNodesCount)
		{
		case 0:
			assert(0); // 間のボーン数が0（ありえない）
			continue;

		case 1: // 間のボーン数が1のときはLookAt
			SolveLookAt(ik);
			break;

		case 2: // 間のボーン数が2のときは余弦定理IK
			SolveCosineIK(ik);
			break;

		default: // 3以上のときはCCD-IK
			SolveCCDIK(ik);
		}
	}
}
