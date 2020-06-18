#include "PMDActor.h"

#include "../Dx12Wrapper/Dx12Wrapper.h"
#include "../Helper/Helper.h"

PMDActor::PMDActor(const std::string & filePath, std::weak_ptr<Dx12Wrapper> dx12, XMFLOAT3 pos)
	: m_dx12{ dx12 }
	, m_pos{ pos }
{
	if (!LoadPMDFile(filePath))
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
}

void PMDActor::Update()
{
	m_angle += 0.01f;
	m_mapTransform->world = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z)
		* XMMatrixRotationY(m_angle);
}

void PMDActor::Draw()
{
	// プリミティブトポロジをセット
	m_dx12.lock()->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点バッファをセット
	m_dx12.lock()->CommandList()->IASetVertexBuffers(0, 1, &m_vbView);

	// インデックスバッファをセット
	m_dx12.lock()->CommandList()->IASetIndexBuffer(&m_ibView);

	// TODO: ワールド行列を入れるヒープを設定
	m_dx12.lock()->CommandList()->SetDescriptorHeaps(1, m_transformHeap.GetAddressOf());

	m_dx12.lock()->CommandList()->SetGraphicsRootDescriptorTable(
		1, m_transformHeap->GetGPUDescriptorHandleForHeapStart());

	// ディスクリプタヒープをセット（マテリアル用）
	m_dx12.lock()->CommandList()->SetDescriptorHeaps(1, m_materialDescHeap.GetAddressOf());

	auto materialH = m_materialDescHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	for (auto& m : m_materials)
	{
		// ルートパラメーターとディスクリプタヒープの関連付け（マテリアル用）
		m_dx12.lock()->CommandList()->SetGraphicsRootDescriptorTable(
			2, materialH);

		// ドローコール
		m_dx12.lock()->CommandList()->DrawIndexedInstanced(m.indicesNum,
			1, idxOffset, 0, 0);

		// ヒープポインターとインデックスを先に進める
		materialH.ptr += m_dx12.lock()->Device()->
			GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
		idxOffset += m.indicesNum;
	}
}

bool PMDActor::LoadPMDFile(const std::string & filePath)
{
	char signature[3]{}; // pmdファイルの先頭3バイトが「Pmd」となっているため、それは構造体に含めない
	FILE* fp;
	PMDHeader pmdHeader;
	unsigned int vertNum; // 頂点数
	unsigned int indicesNum; // インデックス数
	unsigned int materialNum; // マテリアル数
	std::vector<PMDMaterial> pmdMaterials;
	unsigned short boneNum;
	std::vector<PMDBone> pmdBones;

	fopen_s(&fp, filePath.c_str(), "rb");
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	fread(&vertNum, sizeof(vertNum), 1, fp);
	m_vertices.resize(vertNum * sizeof(PMDVertex));
	fread(m_vertices.data(), m_vertices.size(), 1, fp);

	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	m_indices.resize(indicesNum);
	fread(m_indices.data(), m_indices.size() * sizeof(m_indices[0]), 1, fp);

	fread(&materialNum, sizeof(materialNum), 1, fp);
	pmdMaterials.resize(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);
	m_materials.resize(pmdMaterials.size());

	fread(&boneNum, sizeof(boneNum), 1, fp);
	pmdBones.resize(boneNum);
	fread(pmdBones.data(), sizeof(PMDBone), pmdBones.size(), fp);

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

	// ボーンノードマップを作る
	for (int i = 0; i < pmdBones.size(); ++i)
	{
		auto& pb = pmdBones[i];
		boneNames[i] = pb.boneName;

		auto& node = m_boneNodeTable[pb.boneName];
		node.boneIdx = i;
		node.startPos = pb.pos;
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

	// 定数バッファ作成
	result = m_dx12.lock()->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(Transform) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_transformBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// マップ
	result = m_transformBuffer->Map(0, nullptr, (void**)&m_mapTransform);

	if (FAILED(result)) return false;

	m_mapTransform->world = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z);

	m_transformBuffer->Unmap(0, nullptr);

	return true;
}

bool PMDActor::CreateTransformBufferView()
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = m_transformBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (sizeof(Transform) + 0xff) & ~0xff;

	// ビュー作成
	m_dx12.lock()->Device()->CreateConstantBufferView(
		&cbvDesc,
		m_transformHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void * PMDActor::Transform::operator new(size_t size)
{
	return _aligned_malloc(size, 16);
}
