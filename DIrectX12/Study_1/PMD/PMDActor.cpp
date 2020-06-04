#include "PMDActor.h"

#include "PMDRenderer.h"
#include "../Dx12Wrapper/Dx12Wrapper.h"
#include "../Helper/Helper.h"

#include <array>

#include <algorithm>
#pragma comment(lib, "winmm.lib")

#include <iostream>
#include <sstream>

namespace // �������O���
{
	// �{�[�����
	enum class BoneType
	{
		Rotation,      // ��]
		RotAndMove,    // ��]���ړ�
		IK,            // IK
		Undefined,     // ����`
		IKChild,       // IK�e���{�[��
		RotationChild, // ��]�e���{�[��
		IKDestination, // IK�ڑ���{�[��
		Invisible      // �����Ȃ��{�[��
	};

	// z�������̕����Ɍ�����s���Ԃ��֐�
	// @param lookat ���������������x�N�g��
	// @param up ��x�N�g��
	// @pamra right �E�x�N�g��
	XMMATRIX LookAtMatrix(const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right)
	{
		// ���������������iz���j
		XMVECTOR vz = lookat;

		// �i���������������������������́j����y���x�N�g��
		XMVECTOR vy = XMVector3Normalize(XMLoadFloat3(&up));

		// �i���������������������������́jy��
		//XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vz, vx));
		XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));
		vy = XMVector3Normalize(XMVector3Cross(vz, vx));

		// LookAt��up�����������������Ă�����right����ɂ��č�蒼��
		if (std::abs(XMVector3Dot(vy, vz).m128_f32[0] == 1.0f))
		{
			// ����x�������`
			vx = XMVector3Normalize(XMLoadFloat3(&right));

			// ����������������������������y�����v�Z
			vy = XMVector3Normalize(XMVector3Cross(vz, vx));

			// �^��x�����v�Z
			vx = XMVector3Normalize(XMVector3Cross(vy, vz));
		}

		XMMATRIX ret = XMMatrixIdentity();
		ret.r[0] = vx;
		ret.r[1] = vy;
		ret.r[2] = vz;

		return ret;
	}

	// ����̃x�N�g�������̕����Ɍ����邽�߂̍s���Ԃ�
	// @param origin ����̃x�N�g��
	// @param lookat ��������������
	// @param up ��x�N�g��
	// @param right �E�x�N�g��
	// @retbal ����̃x�N�g�������̕����Ɍ������邽�߂̍s��
	XMMATRIX LookAtMatrix(const XMVECTOR& origin, const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right)
	{
		return XMMatrixTranspose(LookAtMatrix(origin, up, right)) * LookAtMatrix(lookat, up, right);
	}
}

HRESULT PMDActor::CreateMaterialData()
{
	// �}�e���A���o�b�t�@�[���쐬
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff;

	auto result = m_dx12->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * m_materials.size()), // ���������Ȃ����d���Ȃ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_materialBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// �}�b�v�}�e���A���ɃR�s�[
	char* mapMaterial = nullptr;
	result = m_materialBuff->Map(0, nullptr, (void**)&mapMaterial);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}


	for (auto& m : m_materials)
	{
		*((MaterialForHlsl*)mapMaterial) = m.material; // �f�[�^�R�s�[
		mapMaterial += materialBuffSize; // ���̃A���C�������g�ʒu�܂Ői�߂�i256�̔{���j
	}

	m_materialBuff->Unmap(0, nullptr);

	return result;
}

HRESULT PMDActor::CreateMaterialAndTextureView()
{
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = m_materials.size() * 5; // �}�e���A�������w��
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

	// �ʏ�e�N�X�`���r���[�쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // �f�t�H���g
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2D�e�N�X�`��
	srvDesc.Texture2D.MipLevels = 1; // �~�b�v�}�b�v�͎g�p���Ȃ��̂� 1

	// �擪���L�^
	auto matDescHeapH = m_materialDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto inc = m_dx12->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < m_materials.size(); ++i)
	{
		// �}�e���A���p�萔�o�b�t�@�[�r���[
		m_dx12->Device()->CreateConstantBufferView(&matCBVDesc, matDescHeapH);

		matDescHeapH.ptr += inc;
		matCBVDesc.BufferLocation += materialBuffSize;

		// �V�F�[�_�[���\�[�X�r���[�i�ǉ��j
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
	// GPU�o�b�t�@�쐬
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

	// �}�b�v�ƃR�s�[
	result = m_transformBuffer->Map(0, nullptr, (void**)&m_mappedMatrices);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}
	m_mappedMatrices[0] = m_transform.world; // 0�Ԃɂ̓��[���h�ϊ��s�������

	copy(m_boneMatrices.begin(), m_boneMatrices.end(), m_mappedMatrices + 1);

	// �r���[�쐬
	D3D12_DESCRIPTOR_HEAP_DESC transformDescHeapDesc{};
	transformDescHeapDesc.NumDescriptors = 1; // �Ƃ肠�������[���h���
	transformDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	transformDescHeapDesc.NodeMask = 0;
	transformDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // �f�B�X�N���v�^�q�[�v���

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
		char model_name[20]; // ���f����
		char comment[256]; // ���f���R�����g
	};

	char signature[3]{}; // �V�O�l�`��
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

	// ���_�֌W
	unsigned int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, fp);

	const unsigned int pmdvertex_size = 38;
	std::vector<unsigned char> vertices(vertNum * pmdvertex_size); // �o�b�t�@�[�̊m��
	fread(vertices.data(), vertices.size(), 1, fp);

	// CD3DX�`���g�������_�o�b�t�@�̍쐬
	auto result = m_dx12->Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // UPLOAD�q�[�v�Ƃ���
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertices.size()), // �T�C�Y�ɉ����ēK�؂Ȑݒ�����Ă����
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

	m_vbView.BufferLocation = m_vb->GetGPUVirtualAddress(); // �o�b�t�@�[�̉��z�A�h���X
	m_vbView.SizeInBytes = vertices.size(); // �S�o�C�g��
	m_vbView.StrideInBytes = pmdvertex_size; // 1���_������̃o�C�g���i�����XMFLOAT3�����Ă������͂��j

	// �C���f�b�N�X�֌W
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

	// ������o�b�t�@�[�ɃC���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappedIdx = nullptr;
	m_ib->Map(0, nullptr, (void**)&mappedIdx);

	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	m_ib->Unmap(0, nullptr);

	m_ibView.BufferLocation = m_ib->GetGPUVirtualAddress();
	m_ibView.Format = DXGI_FORMAT_R16_UINT; // unsigned int�̃C���f�b�N�X�z�񂾂��炱�̐ݒ�
	m_ibView.SizeInBytes = indices.size() * sizeof(indices[0]);

	// �}�e���A���֌W
#pragma pack(1) // ��������1�o�C�g�p�b�L���O�ɂȂ�A�A���C�������g�͔������Ȃ�

	// PMD �}�e���A���\����
	struct PMDMaterial
	{
		XMFLOAT3 diffuse; //�@�f�B�t���[�Y�F
		float alpha; // �f�B�t���[�Y��
		float specularity; // �X�y�L�����̋���
		XMFLOAT3 specular; // �X�y�L�����F
		XMFLOAT3 ambient; // �A���r�G���g�F
		unsigned char toonIdx; // �g�D�[���ԍ�
		unsigned char edgeFlag; // �}�e���A�����Ƃ̗֊s���t���O

		// ���ӁF������2�o�C�g�̃p�f�B���O������I�I

		// 1�o�C�g�p�b�L���O�ɂ���ăp�f�B���O���������Ȃ��I�I

		unsigned int indicesNum; // ���̃}�e���A�������蓖�Ă���C���f�b�N�X��

		char texFilePath[20]; // �e�N�X�`���t�@�C���p�X + ��
	}; // 70�o�C�g�̂͂������A�p�f�B���O���������邽��72�o�C�g�ɂȂ�

#pragma pack() // �p�b�L���O�w��������i�f�t�H���g�ɖ߂��j

	unsigned int materialNum;
	// �}�e���A���ǂݍ���
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

	// �R�s�[
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
		// �g�D�[�����\�[�X�̓ǂݍ���
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
		{ // �X�v���b�^������
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
			// ���f���ƃe�N�X�`���p�X����A�v���P�[�V��������̃e�N�X�`���p�X�𓾂�
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

	// �ǂݍ��ݗp�{�[���\����
	struct PMDBone
	{
		char boneName[20]; // �{�[����
		unsigned short parentNo; // �e�{�[����
		unsigned short nextNo; // ��[�{�[���ԍ�
		unsigned char type; // �{�[�����
		unsigned short ikBoneNo; // IK�{�[���ԍ�
		XMFLOAT3 pos; // �{�[���̊�_���W
	};

#pragma pack()

	unsigned short boneNum = 0;
	fread(&boneNum, sizeof(boneNum), 1, fp);

	std::vector<PMDBone> pmdBones(boneNum);
	fread(pmdBones.data(), sizeof(PMDBone), boneNum, fp);

	// �C���f�b�N�X�Ɩ��O�̑Ή��֌W�\�z�̂��߂ɂ��ƂŎg��
	std::vector<std::string> boneNames(pmdBones.size());

	// IK���̓ǂݍ���
	uint16_t ikNum = 0;
	fread(&ikNum, sizeof(ikNum), 1, fp);

	m_pmdIkData.resize(ikNum);

	for (auto& ik : m_pmdIkData)
	{
		fread(&ik.boneIdx, sizeof(ik.boneIdx), 1, fp);
		fread(&ik.targetIdx, sizeof(ik.targetIdx), 1, fp);

		uint8_t chainLen = 0; // �Ԃɂ����m�[�h�����邩
		fread(&chainLen, sizeof(chainLen), 1, fp);

		ik.nodeIdxes.resize(chainLen);

		fread(&ik.iterations, sizeof(ik.iterations), 1, fp);
		fread(&ik.limit, sizeof(ik.limit), 1, fp);

		if (chainLen == 0)
		{
			continue; // �Ԃ̃m�[�h����0�Ȃ�΂����ŏI���
		}

		fread(ik.nodeIdxes.data(), sizeof(ik.nodeIdxes[0]), chainLen, fp);
	}

	fclose(fp);

	// �ǂݍ��݌�̏���

	m_boneNameArray.resize(pmdBones.size());
	m_boneNodeAddressArray.resize(pmdBones.size());

	m_boneMatrices.resize(pmdBones.size());

	// �{�[�������ׂď���������
	std::fill(
		m_boneMatrices.begin(),
		m_boneMatrices.end(),
		XMMatrixIdentity());

	m_kneeIdxes.clear();
	// �{�[���m�[�h�}�b�v�����
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

		// �C���f�b�N�X���猟�����₷���悤��
		m_boneNameArray[i] = pb.boneName;
		m_boneNodeAddressArray[i] = &node;

		std::string boneName = pb.boneName;
		if (boneName.find("�Ђ�") != std::string::npos)
		{
			m_kneeIdxes.emplace_back(i);
		}
	}

	// �e�q�֌W���\�z
	for (auto& pb : pmdBones)
	{
		// �e�C���f�b�N�X���`�F�b�N�i���肦�Ȃ��ԍ��Ȃ��΂��j
		if (pb.parentNo >= pmdBones.size())
		{
			continue;
		}

		auto parentName = boneNames[pb.parentNo];
		m_boneNodeTable[parentName].children.emplace_back(&m_boneNodeTable[pb.boneName]);
	}

	// �f�o�b�O�p
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
	//	oss << "IK�{�[���ԍ� = " << ik.boneIdx << " : " << getNameFromIdx(ik.boneIdx) << std::endl;

	//	for (auto& node : ik.nodeIdxes)
	//	{
	//		oss << "\t�m�[�h�{�[�� = " << node << " : " << getNameFromIdx(node) << std::endl;
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

	// ���[�v�̂��߂̒ǉ��R�[�h
	if (frameNo > m_duration)
	{
		m_startTime = timeGetTime();
		frameNo = 0;
	}


	// �s����N���A
	// �i�N���A���Ă��Ȃ��ƑO�t���[���̃|�[�Y���d�ˊ|�������
	// ���f��������j
	std::fill(m_boneMatrices.begin(), m_boneMatrices.end(), XMMatrixIdentity());

	// ���[�V�����f�[�^�X�V
	for (auto& boneMotion : m_motionData)
	{
		auto node = m_boneNodeTable[boneMotion.first];

		// ���v������̂�T��
		auto motions = boneMotion.second;
		auto rit = std::find_if(
			motions.rbegin(), motions.rend(),
			[frameNo](const Motion& motion) // �����_��
		{
			return motion.frameNo <= frameNo;
		});

		// ���v������̂��Ȃ���Ώ������΂�
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

	RecursiveMatrixMultiply(&m_boneNodeTable["�Z���^�["], XMMatrixIdentity());

	IKSolve(frameNo);

	copy(m_boneMatrices.begin(), m_boneMatrices.end(), m_mappedMatrices + 1);

}

// �덷�͈͓̔����ǂ����Ɏg�p����萔
constexpr float epsilon = 0.0005f;

float PMDActor::GetYFromXOnBezier(float x, const XMFLOAT2 & a, const XMFLOAT2 & b, uint8_t n)
{
	if (a.x == a.y && b.x == b.y)
	{
		return x; // �v�Z�s�v
	}

	float t = x;
	const float k0 = 1 + 3 * a.x - 3 * b.x; // t~3�̌W��
	const float k1 = 3 * b.x - 6 * a.x;     // t~2�̌W��
	const float k2 = 3 * a.x;               // t�̌W��

	// t���ߎ��ŋ��߂�
	for (int i = 0; i < n; ++i)
	{
		// f(t)�����߂�
		auto ft = k0 * t * t * t + k1 * t * t + k2 * t - x;

		// �������ʂ�0�ɋ߂��i�덷�͈͓̔��j�Ȃ�ł��؂�
		if (ft <= epsilon && ft >= epsilon)
		{
			break;
		}

		t -= ft / 2; // ����
	}

	// ���߂��� t �͂��łɋ��߂Ă���̂� y ���v�Z����
	auto r = 1 - t;
	return t * t * t + 3 * t * t * r * b.y + 3 * t * r * r * a.y;
}

void PMDActor::SolveCCDIK(const PMDIK & ik)
{
	// �^�[�Q�b�g
	auto targetBoneNode = m_boneNodeAddressArray[ik.boneIdx];
	auto targetOriginPos = XMLoadFloat3(&targetBoneNode->startPos);

	auto parentMat = m_boneMatrices[m_boneNodeAddressArray[ik.boneIdx]->ikParentBone];
	XMVECTOR det;
	auto invParentMat = XMMatrixInverse(&det, parentMat);
	auto targetNextPos = XMVector3Transform(targetOriginPos, m_boneMatrices[ik.boneIdx] * invParentMat);

	// ���[�m�[�h
	std::vector<XMVECTOR> bonePositions;
	auto endPos = XMLoadFloat3(&m_boneNodeAddressArray[ik.targetIdx]->startPos);

	// ���ԃm�[�h�i���[�g���܂ށj
	for (auto& cidx : ik.nodeIdxes)
	{
		bonePositions.push_back(XMLoadFloat3(&m_boneNodeAddressArray[cidx]->startPos));
	}

	// ��]�s��L�^
	std::vector<XMMATRIX> mats(bonePositions.size());
	fill(mats.begin(), mats.end(), XMMatrixIdentity());

	auto ikLimit = ik.limit * XM_PI;

	// ik�ɐݒ肳��Ă��鎎�s�񐔂����J��Ԃ�
	for (int c = 0; c < ik.iterations; ++c)
	{
		// �^�[�Q�b�g�Ɩ��[���قڈ�v�����甲����
		if (XMVector3Length(XMVectorSubtract(endPos, targetNextPos)).m128_f32[0] <= epsilon)
		{
			break;
		}

		// ���ꂼ��̃{�[���������̂ڂ�Ȃ���
		// �p�x�����Ɉ���������Ȃ��悤�ɋȂ��Ă���

		// bonePositions��CCD_IK�ɂ�����e�m�[�h�̍��W��
		// �x�N�^�z��ɂ������̂ł�
		for (int bidx = 0; bidx < bonePositions.size(); ++bidx)
		{
			const auto& pos = bonePositions[bidx];

			// �Ώۃm�[�h���疖�[�m�[�h�܂ł�
			// �Ώۃm�[�h����^�[�Q�b�g�܂ł̃x�N�g���쐬
			auto vecToEnd = XMVectorSubtract(endPos, pos); // ���[��
			auto vecToTarget = XMVectorSubtract(targetNextPos, pos); // �^�[�Q�b�g��

			// �������K��
			vecToEnd = XMVector3Normalize(vecToEnd);
			vecToTarget = XMVector3Normalize(vecToTarget);

			// �قړ����x�N�g���ɂȂ��Ă��܂����ꍇ��
			// �O�ϐ����Ȃ����ߎ��̃{�[���Ɉ����n��
			if (XMVector3Length(XMVectorSubtract(vecToEnd, vecToTarget)).m128_f32[0] <= epsilon)
			{
				continue;
			}

			// �O�όv�Z����ъp�x�v�Z
			auto cross = XMVector3Normalize(XMVector3Cross(vecToEnd, vecToTarget)); // ���ɂȂ�

			// �֗��Ȋ֐��������g��cos�i���ϒl�j�Ȃ̂�
			// 0�`90����0�`-90���̂̋�ʂ��Ȃ�
			float angle = XMVector3AngleBetweenVectors(vecToEnd, vecToTarget).m128_f32[0];

			// ��]�̌��E�𒴂��Ă��܂����Ƃ��͌��E�l�ɕ␳
			angle = min(angle, ikLimit);
			XMMATRIX rot = XMMatrixRotationAxis(cross, angle); // ��]�s��쐬

			// ���_���S�ł͂Ȃ��Apos���S�ɉ�]
			auto mat = XMMatrixTranslationFromVector(-pos)
				* rot
				* XMMatrixTranslationFromVector(pos);

			// ��]�s���ێ����Ă����i��Z�ŉ�]�d�ˊ|��������Ă����j
			mats[bidx] *= mat;

			// �ΏۂƂȂ�_�����ׂĉ�]������i���݂̓_���猩�Ė��[������]�j
			// �Ȃ��A��������]������K�v�͂Ȃ�
			for (auto idx = bidx - 1; idx >= 0; --idx)
			{
				bonePositions[idx] = XMVector3Transform(bonePositions[idx], mat);
			}

			endPos = XMVector3Transform(endPos, mat);

			// ���������ɋ߂��Ȃ��Ă����烋�[�v�𔲂���
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
	// IK�\���_��ۑ�
	std::vector<XMVECTOR> positions;

	// IK�̂��ꂼ��̃{�[���Ԃ̋�����ۑ�
	std::array<float, 2> edgeLens;

	// �^�[�Q�b�g�i���[�{�[���ł͂Ȃ��A���[�{�[�����߂Â�
	//             �ڕW�{�[���̍��W���擾�j
	auto& targetNode = m_boneNodeAddressArray[ik.boneIdx];
	auto targetPos = XMVector3Transform(
		XMLoadFloat3(&targetNode->startPos),
		m_boneMatrices[ik.boneIdx]);

	// IK�`�F�[�����t���Ȃ̂ŁA�t�ɕ��Ԃ悤�ɂ��Ă���

	// ���[�{�[��
	auto endNode = m_boneNodeAddressArray[ik.targetIdx];
	positions.emplace_back(XMLoadFloat3(&endNode->startPos));

	// ���Ԃ���у��[�g�{�[��
	for (auto& chainBoneIdx : ik.nodeIdxes)
	{
		auto boneNode = m_boneNodeAddressArray[chainBoneIdx];
		positions.emplace_back(XMLoadFloat3(&boneNode->startPos));
	}

	// �킩��Â炢�̂ŋt�ɂ���
	// ���Ȃ��l�͂��̂܂܌v�Z���Ă��܂�Ȃ�
	std::reverse(positions.begin(), positions.end());

	// ���̒����𑪂��Ă���
	edgeLens[0] = XMVector3Length(
		XMVectorSubtract(positions[1], positions[0])).m128_f32[0];

	edgeLens[1] = XMVector3Length(
		XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	// ���[�g�{�[�����W�ϊ��i�t���ɂȂ��Ă��邽�ߎg�p����C���f�b�N�X�ɒ���
	positions[0] = XMVector3Transform(positions[0], m_boneMatrices[ik.nodeIdxes[1]]);

	// �^�񒆂͎����v�Z�����̂Ōv�Z���Ȃ�

	// ��[�{�[��
	positions[2] = XMVector3Transform(positions[2], m_boneMatrices[ik.boneIdx]);

	// ���[�g����擪�ւ̃x�N�g��������Ă���
	auto linearVec = XMVectorSubtract(positions[2], positions[0]);

	float a = XMVector3Length(linearVec).m128_f32[0];
	float b = edgeLens[0];
	float c = edgeLens[1];

	linearVec = XMVector3Normalize(linearVec);

	// ���[�g����^�񒆂ւ̊p�x�v�Z
	float theta1 = acosf((a * a + b * b - c * c) / (2 * a * b));

	// �^�񒆂���^�[�Q�b�g�ւ̊p�x�v�Z
	float theta2 = acosf((b * b + c * c - a * a) / (2 * b * c));

	// �u�������߂�v
	// �����^�񒆂��u�Ђ��v�ł������ꍇ�ɂ͋����I��X��������
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

	// ���ӓ_�FIK�`�F�[���̓��[�g�Ɍ������Ă��琔�����邽�� 1 �����[�g�ɋ߂�
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
	// ���̊֐��ɗ������_�Ńm�[�h��1�����Ȃ��A
	// �`�F�[���ɓ����Ă���̃I�[�h�ԍ���IK�̃��[�g�m�[�h�̂��̂Ȃ̂ŁA
	// ���̃��[�g�m�[�h���疖�[�Ɍ������x�N�g�����l����
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
	// �O�ɂ��s�����悤�ɁAIK�I��/�I�t�����t���[���ԍ��ŋt���猟��
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
				if (!ikEnableIt->second) // �����I�t�Ȃ�ł��؂�
				{
					continue;
				}
			}
		}

		auto childrenNodesCount = ik.nodeIdxes.size();

		switch (childrenNodesCount)
		{
		case 0: // �Ԃ̃{�[������ 0�i���肦�Ȃ��j
			assert(0);
			continue;

		case 1: // �Ԃ̃{�[������ 1 �̂Ƃ���LookAt
			SolveLookAt(ik);
			break;

		case 2: // �Ԃ̃{�[������ 2 �̂Ƃ��͗]���藝IK
			SolveCosineIK(ik);
			break;

		default: // 3 �ȏ�̂Ƃ��� CCD_IK
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
	fseek(fp, 50, SEEK_SET); // �ŏ��̃o�C�g�͔�΂���OK

	unsigned int motionDataNum = 0;
	fread(&motionDataNum, sizeof(motionDataNum), 1, fp);

	struct VMDMotion
	{
		char boneName[15]; // �{�[����
		unsigned int frameNo; // �t���[���ԍ�
		XMFLOAT3 location; // �ʒu
		XMFLOAT4 quaternion; // �N�H�[�^�j�I��
		unsigned char bezier[64]; // [4][4][4]�x�W�F��ԃp�����[�^�[
	};

	std::vector<VMDMotion> vmdMotionData(motionDataNum);

	for (auto& motion : vmdMotionData)
	{
		fread(motion.boneName, sizeof(motion.boneName), 1, fp); // �{�[����
		fread(&motion.frameNo,
			sizeof(motion.frameNo)      // �t���[���ԍ�
			+ sizeof(motion.location)   // �ʒu�iIK�̂Ƃ��Ɏg�p�\��j
			+ sizeof(motion.quaternion) // �N�H�[�^�j�I��
			+ sizeof(motion.bezier),    // ��ԃx�W�F�f�[�^
			1, fp);
	}

#pragma pack(1)

	// �\��f�[�^
	struct VMDMorph
	{
		char name[15]; // ���O�i�p�f�B���O���Ă��܂��j
		uint32_t frameNo; // �t���[���ԍ�
		float weight; // �E�F�C�g�i0.0f�`1.0f�j
	}; // �S����23�o�C�g�Ȃ̂�#pragma pack�œǂ�

#pragma pack()

	uint32_t morphCount = 0;
	fread(&morphCount, sizeof(morphCount), 1, fp);

	std::vector<VMDMorph> morphs(morphCount);
	fread(morphs.data(), sizeof(VMDMorph), morphCount, fp);

#pragma pack(1)

	// �J����
	struct VMDCamera
	{
		uint32_t framNo; // �t���[���ԍ�
		float distance; // ����
		XMFLOAT3 pos; // ���W
		XMFLOAT3 eulerAngle; // �I�C���[�p
		uint8_t interpolation[24]; // ���
		uint32_t fov; // ����p
		uint8_t persFlg; // �p�[�X�t���O ON/OFF
	}; // 61�o�C�g�i�����#pragma pack�̕K�v����j

#pragma pack()

	uint32_t vmdCameraCount = 0;
	fread(&vmdCameraCount, sizeof(vmdCameraCount), 1, fp);

	std::vector<VMDCamera> cameraData(vmdCameraCount);
	fread(cameraData.data(), sizeof(VMDCamera), vmdCameraCount, fp);

	// ���C�g�ؖ��f�[�^
	struct VMDLight
	{
		uint32_t frameNo; // �t���[���ԍ�
		XMFLOAT3 rgb; // ���C�g�F
		XMFLOAT3 vec; // �����x�N�g���i���s�����j
	};

	uint32_t vmdLightCount = 0;
	fread(&vmdLightCount, sizeof(vmdLightCount), 1, fp);

	std::vector<VMDLight> lights(vmdLightCount);
	fread(lights.data(), sizeof(VMDLight), vmdLightCount, fp);

#pragma pack(1)

	// �Z���t�e�f�[�^
	struct VMDSelfShadow
	{
		uint32_t frameNo; // �t���[���ԍ�
		uint8_t mode; // �e���[�h�i0:�e�Ȃ��A1:���[�h1�A2:���[�h2�j
		float distance; // ����
	};

#pragma pack()

	uint32_t selfShadowCount = 0;
	fread(&selfShadowCount, sizeof(selfShadowCount), 1, fp);

	std::vector<VMDSelfShadow> selfShadowData(selfShadowCount);
	fread(selfShadowData.data(), sizeof(VMDSelfShadow), selfShadowCount, fp);

	// IK�I��/�I�t�̐؂�ւ�萔
	uint32_t ikSwitchCount = 0;
	fread(&ikSwitchCount, sizeof(ikSwitchCount), 1, fp);

	m_ikEnableData.resize(ikSwitchCount);

	for (auto& ikEnable : m_ikEnableData)
	{
		// �L�[�t���[�����Ȃ̂ł܂��̓t���[���ԍ��ǂݍ���
		fread(&ikEnable.frameNo, sizeof(ikEnable.frameNo), 1, fp);

		// ���ɉَq�t���O�����邪�A����͎g�p���Ȃ�����
		// 1�o�C�g�V�[�N�ł����܂�Ȃ�
		uint8_t visibleFlg = 0;
		fread(&visibleFlg, sizeof(visibleFlg), 1, fp);

		// �Ώۃ{�[�����ǂݍ���
		uint32_t ikBoneCount = 0;
		fread(&ikBoneCount, sizeof(ikBoneCount), 1, fp);

		// ���[�v�����O��ON/OFF�����擾
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

	// VMD�̃��[�V�����f�[�^����A���ۂɎg�p���郂�[�V�����e�[�u���֕ϊ�
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



	RecursiveMatrixMultiply(&m_boneNodeTable["�Z���^�["], XMMatrixIdentity());

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
	unsigned int idxOffset = 0; // �ŏ��̓I�t�Z�b�g�Ȃ�

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

			// �q�[�v�|�C���^�[�ƃC���f�b�N�X�����ɐi�߂�
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
