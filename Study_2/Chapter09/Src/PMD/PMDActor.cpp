#include "PMDActor.h"

#include "../Dx12Wrapper/Dx12Wrapper.h"
#include "../Helper/Helper.h"

PMDActor::PMDActor(const std::string & filePath, std::weak_ptr<Dx12Wrapper> dx12)
	: m_dx12{ dx12 }
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
}

void PMDActor::Draw()
{
	// プリミティブトポロジをセット
	m_dx12.lock()->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点バッファをセット
	m_dx12.lock()->CommandList()->IASetVertexBuffers(0, 1, &vbView);

	// インデックスバッファをセット
	m_dx12.lock()->CommandList()->IASetIndexBuffer(&ibView);

	// ディスクリプタヒープをセット（マテリアル用）
	m_dx12.lock()->CommandList()->SetDescriptorHeaps(1, materialDescHeap.GetAddressOf());

	auto materialH = materialDescHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	for (auto& m : materials)
	{
		// ルートパラメーターとディスクリプタヒープの関連付け（マテリアル用）
		m_dx12.lock()->CommandList()->SetGraphicsRootDescriptorTable(
			1, materialH);

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

	fopen_s(&fp, filePath.c_str(), "rb");
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	fread(&vertNum, sizeof(vertNum), 1, fp);
	vertices.resize(vertNum * sizeof(PMDVertex));
	fread(vertices.data(), vertices.size(), 1, fp);

	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	fread(&materialNum, sizeof(materialNum), 1, fp);
	pmdMaterials.resize(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp);

	fclose(fp);

	materials.resize(pmdMaterials.size());

	// コピー
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		materials[i].indicesNum = pmdMaterials[i].indicesNum;
		materials[i].material.diffuse = pmdMaterials[i].diffuse;
		materials[i].material.alpha = pmdMaterials[i].alpha;
		materials[i].material.specular = pmdMaterials[i].specular;
		materials[i].material.specularity = pmdMaterials[i].specularity;
		materials[i].material.ambient = pmdMaterials[i].ambient;
	}

	texResources.resize(materialNum);
	sphResources.resize(materialNum);
	spaResources.resize(materialNum);
	toonResources.resize(materialNum);

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
		toonResources[i] = m_dx12.lock()->GetTextureByPath(toonFilePath);

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

			texResources[i] = m_dx12.lock()->GetTextureByPath(texFilePath);
		}
		if (sphFileName != "")
		{
			auto sphFilePath = GetTexturePathFromModelAndTexPath(
				filePath,
				sphFileName.c_str());

			sphResources[i] = m_dx12.lock()->GetTextureByPath(sphFilePath);
		}
		if (spaFileName != "")
		{
			auto spaFilePath = GetTexturePathFromModelAndTexPath(
				filePath,
				spaFileName.c_str());

			spaResources[i] = m_dx12.lock()->GetTextureByPath(spaFilePath);
		}
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
		&CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * materials.size()), // もったいないやり方
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(materialBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// マップマテリアルにコピー
	char* mapMaterial{ nullptr };
	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);

	if (FAILED(result)) return false;

	for (auto& m : materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material; // データコピー
		mapMaterial += materialBuffSize; // 次のアラインメント位置まで進める（256の倍数）
	}

	materialBuff->Unmap(0, nullptr);

	return true;
}

bool PMDActor::CreateMaterialBufferView()
{
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc{};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = materials.size() * 5; // マテリアル数 * 5を指定
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = m_dx12.lock()->Device()->CreateDescriptorHeap(&matDescHeapDesc,
		IID_PPV_ARGS(materialDescHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// マテリアル用定数バッファービュー作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc{};
	matCBVDesc.BufferLocation = materialBuff->GetGPUVirtualAddress(); // バッファアドレス
	matCBVDesc.SizeInBytes = AlignmentedSize(sizeof(MaterialForHlsl),
		D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT); // マテリアルの256アラインメントサイズ

	// マテリアル用シェーダーリソースビュー作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // デフォルト
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1; // ミップマップは使用しないので 1

	auto matDescHeapH = materialDescHeap->GetCPUDescriptorHandleForHeapStart(); // 先頭を記録
	auto inc = m_dx12.lock()->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < materials.size(); ++i)
	{
		// 定数バッファービュー
		m_dx12.lock()->Device()->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += inc;
		matCBVDesc.BufferLocation += AlignmentedSize(sizeof(MaterialForHlsl),
			D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		// シェーダーリソースビュー
		if (texResources[i] != nullptr)
		{
			srvDesc.Format = texResources[i]->GetDesc().Format;
			m_dx12.lock()->Device()->CreateShaderResourceView(
				texResources[i].Get(),
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

		if (sphResources[i] != nullptr)
		{
			srvDesc.Format = sphResources[i]->GetDesc().Format;
			m_dx12.lock()->Device()->CreateShaderResourceView(
				sphResources[i].Get(),
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

		if (spaResources[i] != nullptr)
		{
			srvDesc.Format = spaResources[i]->GetDesc().Format;
			m_dx12.lock()->Device()->CreateShaderResourceView(
				spaResources[i].Get(),
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

		if (toonResources[i] != nullptr)
		{
			srvDesc.Format = toonResources[i]->GetDesc().Format;
			m_dx12.lock()->Device()->CreateShaderResourceView(
				toonResources[i].Get(),
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
		&CD3DX12_RESOURCE_DESC::Buffer(vertices.size()), // サイズ変更
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	unsigned char* vertMap{ nullptr };
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);

	if (FAILED(result)) return false;

	std::copy(vertices.begin(), vertices.end(), vertMap);
	vertBuff->Unmap(0, nullptr);

	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress(); // バッファの仮想アドレス
	vbView.SizeInBytes = vertices.size(); // 全バイト数
	vbView.StrideInBytes = sizeof(PMDVertex); // 1頂点あたりのバイト数

	return true;
}

bool PMDActor::CreateIndexBuffer()
{
	auto result = m_dx12.lock()->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);

	// 作ったバッファーにインデックスデータをコピー
	unsigned short* mappedIdx{ nullptr };
	result = idxBuff->Map(0, nullptr, (void**)&mappedIdx);

	if (FAILED(result)) return false;

	std::copy(indices.begin(), indices.end(), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	return true;
}
