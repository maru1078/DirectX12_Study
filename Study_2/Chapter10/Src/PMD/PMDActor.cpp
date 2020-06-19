#include "PMDActor.h"

#include <algorithm>
#include "../Dx12Wrapper/Dx12Wrapper.h"
#include "../Helper/Helper.h"

#pragma comment(lib, "winmm.lib") // timeGetTime()���ĂԂ̂ɕK�v

PMDActor::PMDActor(const std::string & filePath, std::weak_ptr<Dx12Wrapper> dx12, XMFLOAT3 pos)
	: m_dx12{ dx12 }
	, m_pos{ pos }
{
	if (!LoadPMDFile(filePath))
	{
		assert(0);
	}
	if (!LoadVMDFile("Motion/motion.vmd"))
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

	// �������烋�[�v�̂��߂̒ǉ��R�[�h
	if (frameNo > m_duration)
	{
		m_startTime = timeGetTime();
		frameNo = 0;
	}

	// �s����̃N���A
	std::fill(m_boneMatrices.begin(), m_boneMatrices.end(), XMMatrixIdentity());

	// ���[�V�����f�[�^�X�V
	for (auto& boneMotion : m_motionData)
	{
		// ���f���f�[�^���ɑ��݂��Ă���{�[�����ǂ������ׂ�
		auto itBoneNode = m_boneNodeTable.find(boneMotion.first);
		
		// ���݂��Ă��Ȃ������ꍇ�A�������΂�
		if (itBoneNode == m_boneNodeTable.end())
		{
			continue;
		}

		auto node = itBoneNode->second;

		// ���v������̂�T��
		auto motions = boneMotion.second;
		auto rit = std::find_if(motions.rbegin(), motions.rend(), 
			[frameNo](const Motion& motion)
		{
			return motion.frameNo <= frameNo;
		});

		// ���v������̂��Ȃ���Ώ������΂�
		if (rit == motions.rend())
		{
			continue;
		}

		XMMATRIX rotation;
		auto it = rit.base();

		if (it != motions.end())
		{
			auto t = static_cast<float>(frameNo - rit->frameNo)
				/ static_cast<float>(it->frameNo - rit->frameNo);

			t = GetYFromXOnBezier(t, it->p1, it->p2, 12);

			rotation = XMMatrixRotationQuaternion(
				XMQuaternionSlerp(rit->quaternion, it->quaternion, t));
		}
		else
		{
			rotation = XMMatrixRotationQuaternion(rit->quaternion);
		}

		auto& pos = node.startPos;
		auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* rotation
			* XMMatrixTranslation(pos.x, pos.y, pos.z);

		m_boneMatrices[node.boneIdx] = mat;
	}

	RecursiveMatrixMultiply(&m_boneNodeTable["�Z���^�["], XMMatrixIdentity());
	std::copy(m_boneMatrices.begin(), m_boneMatrices.end(), m_mappedMatrices + 1);
}

void PMDActor::Update()
{
	MotionUpdate();

	//m_angle += 0.01f;
	m_mappedMatrices[0] = XMMatrixTranslation(m_pos.x, m_pos.y, m_pos.z)
		* XMMatrixRotationY(m_angle);
}

void PMDActor::Draw()
{
	// �v���~�e�B�u�g�|���W���Z�b�g
	m_dx12.lock()->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ���_�o�b�t�@���Z�b�g
	m_dx12.lock()->CommandList()->IASetVertexBuffers(0, 1, &m_vbView);

	// �C���f�b�N�X�o�b�t�@���Z�b�g
	m_dx12.lock()->CommandList()->IASetIndexBuffer(&m_ibView);

	// TODO: ���[���h�s�������q�[�v��ݒ�
	m_dx12.lock()->CommandList()->SetDescriptorHeaps(1, m_transformHeap.GetAddressOf());

	m_dx12.lock()->CommandList()->SetGraphicsRootDescriptorTable(
		1, m_transformHeap->GetGPUDescriptorHandleForHeapStart());

	// �f�B�X�N���v�^�q�[�v���Z�b�g�i�}�e���A���p�j
	m_dx12.lock()->CommandList()->SetDescriptorHeaps(1, m_materialDescHeap.GetAddressOf());

	auto materialH = m_materialDescHeap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	for (auto& m : m_materials)
	{
		// ���[�g�p�����[�^�[�ƃf�B�X�N���v�^�q�[�v�̊֘A�t���i�}�e���A���p�j
		m_dx12.lock()->CommandList()->SetGraphicsRootDescriptorTable(
			2, materialH);

		// �h���[�R�[��
		m_dx12.lock()->CommandList()->DrawIndexedInstanced(m.indicesNum,
			1, idxOffset, 0, 0);

		// �q�[�v�|�C���^�[�ƃC���f�b�N�X���ɐi�߂�
		materialH.ptr += m_dx12.lock()->Device()->
			GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
		idxOffset += m.indicesNum;
	}
}

bool PMDActor::LoadPMDFile(const std::string & filePath)
{
	char signature[3]{}; // pmd�t�@�C���̐擪3�o�C�g���uPmd�v�ƂȂ��Ă��邽�߁A����͍\���̂Ɋ܂߂Ȃ�
	FILE* fp;
	PMDHeader pmdHeader;
	unsigned int vertNum; // ���_��
	unsigned int indicesNum; // �C���f�b�N�X��
	unsigned int materialNum; // �}�e���A����
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

	// �R�s�[
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
		// �g�D�[�����\�[�X�̓ǂݍ���
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
		{ // �X�v���b�^������
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

		// ���f���ƃe�N�X�`���p�X����A�v���P�[�V��������̃e�N�X�`���p�X�𓾂�
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
	// |     �{�[�������m�[�h�e�[�u����     |
	// +--------------------------------------+

	// �C���f�b�N�X�Ɩ��O�̑Ή��֌W�\�z�̂��߂ɂ��ƂŎg��
	std::vector<std::string> boneNames{ pmdBones.size() };

	// �{�[���m�[�h�}�b�v�����
	for (int i = 0; i < pmdBones.size(); ++i)
	{
		auto& pb = pmdBones[i];
		boneNames[i] = pb.boneName;

		auto& node = m_boneNodeTable[pb.boneName];
		node.boneIdx = i;
		node.startPos = pb.pos;
	}

	// �e�q�֌W���\�z����
	for (auto& pb : pmdBones)
	{
		// �e�C���f�b�N�X���`�F�b�N�i���肦�Ȃ������Ȃ��΂��j
		if (pb.parentNo >= pmdBones.size())
		{
			continue;
		}

		// �e�ԍ�����e�̃{�[�������擾
		auto parentName = boneNames[pb.parentNo];

		// �e�̎q�m�[�h���ɒǉ�
		m_boneNodeTable[parentName].children.emplace_back(
			&m_boneNodeTable[pb.boneName]);
	}

	m_boneMatrices.resize(pmdBones.size());

	// �{�[�������ׂď���������
	std::fill(m_boneMatrices.begin(), m_boneMatrices.end(), XMMatrixIdentity());

	return true;
}

bool PMDActor::LoadVMDFile(const std::string & filePath)
{
	FILE* fp;
	unsigned int motionDataNum;
	std::vector<VMDMotion> vmdMotionData;

	fopen_s(&fp, filePath.c_str(), "rb");
	fseek(fp, 50, SEEK_SET); // �ŏ���50�o�C�g�͔�΂�

	fread(&motionDataNum, sizeof(motionDataNum), 1, fp);
	vmdMotionData.resize(motionDataNum);

	for (auto& motion : vmdMotionData)
	{
		fread(motion.boneName, sizeof(motion.boneName), 1, fp); // �{�[����
		fread(&motion.frameNo,
			sizeof(motion.frameNo)		// �t���[���ԍ�
			+ sizeof(motion.location)	// �ʒu
			+ sizeof(motion.quaternion) // �N�H�[�^�j�I��
			+ sizeof(motion.bezier),	// ��ԃx�W�F�f�[�^
			1, fp);

		m_duration = std::max<unsigned int>(m_duration, motion.frameNo);
	}

	// VMD���[�V�����f�[�^����A���ۂɎg�p���郂�[�V�����e�[�u���֕ϊ�
	for (auto& vmdMotion : vmdMotionData)
	{
		m_motionData[vmdMotion.boneName].emplace_back(
			Motion(vmdMotion.frameNo,
				XMLoadFloat4(&vmdMotion.quaternion),
				XMFLOAT2((float)vmdMotion.bezier[3] / 127.0f, (float)vmdMotion.bezier[7] / 127.0f),
				XMFLOAT2((float)vmdMotion.bezier[11] / 127.0f, (float)vmdMotion.bezier[15] / 127.0f)));
	}

	// �t���[���ԍ��ɂ��\�[�g
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
	// �}�e���A���o�b�t�@�[�쐬
	auto materialBuffSize = (sizeof(MaterialForHlsl) + 0xff) & ~0xff;

	auto result = m_dx12.lock()->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * m_materials.size()), // ���������Ȃ�����
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_materialBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// �}�b�v�}�e���A���ɃR�s�[
	char* mapMaterial{ nullptr };
	result = m_materialBuff->Map(0, nullptr, (void**)&mapMaterial);

	if (FAILED(result)) return false;

	for (auto& m : m_materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material; // �f�[�^�R�s�[
		mapMaterial += materialBuffSize; // ���̃A���C�������g�ʒu�܂Ői�߂�i256�̔{���j
	}

	m_materialBuff->Unmap(0, nullptr);

	return true;
}

bool PMDActor::CreateMaterialBufferView()
{
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc{};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = m_materials.size() * 5; // �}�e���A���� * 5���w��
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = m_dx12.lock()->Device()->CreateDescriptorHeap(&matDescHeapDesc,
		IID_PPV_ARGS(m_materialDescHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// �}�e���A���p�萔�o�b�t�@�[�r���[�쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc{};
	matCBVDesc.BufferLocation = m_materialBuff->GetGPUVirtualAddress(); // �o�b�t�@�A�h���X
	matCBVDesc.SizeInBytes = AlignmentedSize(sizeof(MaterialForHlsl),
		D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT); // �}�e���A����256�A���C�������g�T�C�Y

	// �}�e���A���p�V�F�[�_�[���\�[�X�r���[�쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // �f�t�H���g
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2D�e�N�X�`��
	srvDesc.Texture2D.MipLevels = 1; // �~�b�v�}�b�v�͎g�p���Ȃ��̂� 1

	auto matDescHeapH = m_materialDescHeap->GetCPUDescriptorHandleForHeapStart(); // �擪���L�^
	auto inc = m_dx12.lock()->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < m_materials.size(); ++i)
	{
		// �萔�o�b�t�@�[�r���[
		m_dx12.lock()->Device()->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += inc;
		matCBVDesc.BufferLocation += AlignmentedSize(sizeof(MaterialForHlsl),
			D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		// �V�F�[�_�[���\�[�X�r���[
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
		&CD3DX12_RESOURCE_DESC::Buffer(m_vertices.size()), // �T�C�Y�ύX
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_vertBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	unsigned char* vertMap{ nullptr };
	result = m_vertBuff->Map(0, nullptr, (void**)&vertMap);

	if (FAILED(result)) return false;

	std::copy(m_vertices.begin(), m_vertices.end(), vertMap);
	m_vertBuff->Unmap(0, nullptr);

	m_vbView.BufferLocation = m_vertBuff->GetGPUVirtualAddress(); // �o�b�t�@�̉��z�A�h���X
	m_vbView.SizeInBytes = m_vertices.size(); // �S�o�C�g��
	m_vbView.StrideInBytes = sizeof(PMDVertex); // 1���_������̃o�C�g��

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

	// ������o�b�t�@�[�ɃC���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappedIdx{ nullptr };
	result = m_idxBuff->Map(0, nullptr, (void**)&mappedIdx);

	if (FAILED(result)) return false;

	std::copy(m_indices.begin(), m_indices.end(), mappedIdx);
	m_idxBuff->Unmap(0, nullptr);

	return true;
}

bool PMDActor::CreateTransformBuffer()
{
	// �q�[�v�쐬
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

	// �萔�o�b�t�@�쐬
	result = m_dx12.lock()->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(buffSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_transformBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// �}�b�v
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

	// �r���[�쐬
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
		return x; // �v�Z�s�v
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x; // t^3�̌W��
	const float k1 = 3 * b.x - 6 * a.x;     // t^2�̌W��
	const float k2 = 3 * a.x;               // t  �̌W��

	// �덷�͈͓̔����ǂ����Ɏg�p����萔
	constexpr float epsilon = 0.0005f;

	// t���ߎ��ŋ��߂�
	for (int i = 0; i < n; ++i)
	{
		// f(t)�����߂�
		auto ft = (k0 * t * t * t) + (k1 * t * t) + (k2 * t) - x;

		// �������ʂ�0�ɋ߂��i�덷�͈͓̔��j�Ȃ�ł��؂�
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}

		t -= ft / 2; // ����
	}

	// ���߂���t�͂��łɋ��߂Ă���̂�y���v�Z����
	auto r = 1 - t;

	return (t * t * t) + (3 * t * t * r * b.y) + (3 * t * r * r * a.y);
}
