#include "PMDActor.h"

#include <algorithm>
#include <sstream>
#include <array>
#include "../Dx12Wrapper/Dx12Wrapper.h"
#include "../Helper/Helper.h"

#pragma comment(lib, "winmm.lib") // timeGetTime()���ĂԂ̂ɕK�v

// �덷�͈͓̔����ǂ����Ɏg�p����萔
constexpr float epsilon = 0.0005f;

// z�����������̕����Ɍ�����s���Ԃ��֐�
// @param lookat ���������������x�N�g��
// @param up ��x�N�g��
// @param right �E�x�N�g��
XMMATRIX LookAtMatrix(const XMVECTOR& lookat, const XMFLOAT3& up, const XMFLOAT3& right)
{
	// ���������������iz���j
	XMVECTOR vz = lookat;

	// �i���������������������������́j����y���x�N�g��
	XMVECTOR vy = XMVector3Normalize(XMLoadFloat3(&up));

	// �i���������������������������́jy��
	XMVECTOR vx = XMVector3Normalize(XMVector3Cross(vy, vz));
	vy = XMVector3Normalize(XMVector3Cross(vz, vx));

	// LookAt��up�����������������Ă�����right����ɂ��č�蒼��
	if (std::abs(XMVector3Dot(vy, vz).m128_f32[0]) == 1.0f)
	{
		// ����x�������`
		vx = XMVector3Normalize(XMLoadFloat3(&right));

		// ����������������������������Y�����v�Z
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

// ����̃x�N�g�������̕����Ɍ�����s���Ԃ��֐�
// @param origin ����̃x�N�g��
// @param lookat ���������������x�N�g��
// @param up ��x�N�g��
// @param right �E�x�N�g��
// @return ����̃x�N�g�������̕����Ɍ����邽�߂̍s��
XMMATRIX LookAtMatrix(const XMVECTOR& origin, const XMVECTOR& lookat, const XMFLOAT3& up, const XMFLOAT3& right)
{
	// ���ǂ��������Ƃ��H
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

	RecursiveMatrixMultiply(&m_boneNodeTable["�Z���^�["], XMMatrixIdentity());

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
	// �v���~�e�B�u�g�|���W���Z�b�g
	m_dx12.lock()->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ���_�o�b�t�@���Z�b�g
	m_dx12.lock()->CommandList()->IASetVertexBuffers(0, 1, &m_vbView);

	// �C���f�b�N�X�o�b�t�@���Z�b�g
	m_dx12.lock()->CommandList()->IASetIndexBuffer(&m_ibView);

	// ���[���h�s�������q�[�v��ݒ�
	m_dx12.lock()->CommandList()->SetDescriptorHeaps(1, m_transformHeap.GetAddressOf());

	m_dx12.lock()->CommandList()->SetGraphicsRootDescriptorTable(
		1, m_transformHeap->GetGPUDescriptorHandleForHeapStart());

	// �f�B�X�N���v�^�q�[�v���Z�b�g�i�}�e���A���p�j
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
			// ���[�g�p�����[�^�[�ƃf�B�X�N���v�^�q�[�v�̊֘A�t���i�}�e���A���p�j
			m_dx12.lock()->CommandList()->SetGraphicsRootDescriptorTable(
				2, materialH);

			// �h���[�R�[��
			m_dx12.lock()->CommandList()->DrawIndexedInstanced(m.indicesNum,
				2, idxOffset, 0, 0);

			// �q�[�v�|�C���^�[�ƃC���f�b�N�X���ɐi�߂�
			materialH.ptr += m_dx12.lock()->Device()->
				GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
			idxOffset += m.indicesNum;
		}
	}
}

bool PMDActor::LoadPMDFile(const std::string & filePath)
{
	char signature[3]{}; // pmd�t�@�C���̐擪3�o�C�g���uPmd�v�ƂȂ��Ă��邽�߁A����͍\���̂Ɋ܂߂Ȃ�
	FILE* fp;
	PMDHeader pmdHeader;
	unsigned int vertNum; // ���_��
	unsigned int materialNum; // �}�e���A����
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
	m_boneNameArray.resize(pmdBones.size());
	m_boneNodeAddressArray.resize(pmdBones.size());
	m_kneeIdxes.clear();

	// �{�[���m�[�h�}�b�v�����
	for (int i = 0; i < pmdBones.size(); ++i)
	{
		auto& pb = pmdBones[i];
		boneNames[i] = pb.boneName;

		auto& node = m_boneNodeTable[pb.boneName];
		node.boneIdx = i;
		node.startPos = pb.pos;

		// �C���f�b�N�X���������₷���悤��
		m_boneNameArray[i] = pb.boneName;
		m_boneNodeAddressArray[i] = &node;

		// �Ђ�������Ă���
		std::string boneName = pb.boneName;

		if (boneName.find("�Ђ�") != std::string::npos)
		{
			m_kneeIdxes.emplace_back(i);
		}
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
	uint32_t morphCount = 0;
	std::vector<VMDMorph> morphs;
	uint32_t vmdCameraCount = 0;
	std::vector<VMDCamera> cameraData;
	uint32_t vmdLightCount = 0;
	std::vector<VMDLight> lights;
	uint32_t selfShadowCount = 0;
	std::vector<VMDSelfShadow> selfShadowData;
	uint32_t ikSwitchCount = 0; // IK�I��/�I�t�̐؂�ւ��

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
		// �L�[�t���[�����Ȃ̂ł܂��̓t���[���ԍ��ǂݍ���
		fread(&ikEnable.frameNo, sizeof(ikEnable.frameNo), 1, fp);

		// ���ɉ��t���O�����邪�A����͎g�p���Ȃ�����
		// 1�o�C�g�V�[�N�ł��\��Ȃ�
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

	// VMD���[�V�����f�[�^����A���ۂɎg�p���郂�[�V�����e�[�u���֕ϊ�
	for (auto& vmdMotion : vmdMotionData)
	{
		m_motionData[vmdMotion.boneName].emplace_back(
			Motion(vmdMotion.frameNo,XMLoadFloat4(&vmdMotion.quaternion), vmdMotion.location,
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

	// ��]���L�^
	std::vector<XMMATRIX> mats(bonePositions.size());
	std::fill(mats.begin(), mats.end(), XMMatrixIdentity());

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

		// bonePositions�́ACCD-IK�ɂ�����e�m�[�h�̍��W��
		// �x�N�^�z��ɂ������́B
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
			// �O�ςł��Ȃ����ߎ��̃{�[���������n��
			if (XMVector3Length(XMVectorSubtract(vecToEnd, vecToTarget)).m128_f32[0] <= epsilon)
			{
				continue;
			}

			// �O�όv�Z����ъp�x�v�Z
			auto cross = XMVector3Normalize(XMVector3Cross(vecToEnd, vecToTarget)); // ���ɂȂ�

			// �֗��Ȋ֐��������g��cos�i���ϒl�j�Ȃ̂�
			// 0 �` 90����0 �` -90���̋�ʂ��Ȃ�
			float angle = XMVector3AngleBetweenVectors(vecToEnd, vecToTarget).m128_f32[0];

			// ��]���E�𒴂��Ă��܂����Ƃ��͌��E�l�ɕ␳
			angle = min(angle, ikLimit);
			XMMATRIX rot = XMMatrixRotationAxis(cross, angle); // ��]�s��쐬

			// ���_���S�ł͂Ȃ�pos���S�ɉ�]
			auto mat = XMMatrixTranslationFromVector(-pos)
				* rot
				* XMMatrixTranslationFromVector(pos);

			// ��]�s���ێ����Ă���
			mats[bidx] *= mat;

			// �ΏۂƂȂ�_�����ׂĉ�]������i���݂̓_���猩�Ė��[������]�j
			// �Ȃ���������]������K�v�͂Ȃ�
			for (auto idx = bidx - 1; idx >= 0; --idx)
			{
				bonePositions[idx] = XMVector3Transform(bonePositions[idx], mat);
			}

			endPos = XMVector3Transform(endPos, mat);

			// ���������ɋ߂��Ȃ����烋�[�v�𔲂���
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

	// �^�[�Q�b�g�i���[�{�[���ł͂Ȃ��A���[�{�[�����߂Â��ڕW�{�[���̍��W���擾�j
	auto& targetNode = m_boneNodeAddressArray[ik.boneIdx];
	auto targetPos = XMVector3Transform(XMLoadFloat3(&targetNode->startPos), m_boneMatrices[ik.boneIdx]);

	// IK�`�F�[�����t���Ȃ̂ŁA�t�ɕ��Ԃ悤�ɂ��Ă���

	// ���[�{�[��
	auto endNode = m_boneNodeAddressArray[ik.targetIdx];
	positions.emplace_back(XMLoadFloat3(&endNode->startPos));

	// ���ԋy�у��[�g�{�[��
	for (auto& chainBoneIdx : ik.nodeIdxes)
	{
		auto boneNode = m_boneNodeAddressArray[chainBoneIdx];
		positions.emplace_back(XMLoadFloat3(&boneNode->startPos));
	}

	// �킩��Â炢�̂ŋt�ɂ���
	std::reverse(positions.begin(), positions.end());

	// ���̒����𑪂��Ă���
	edgeLens[0] = XMVector3Length(XMVectorSubtract(positions[1], positions[0])).m128_f32[0];
	edgeLens[1] = XMVector3Length(XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	// ���[�g�{�[�����W�ϊ��i�t���ɂȂ��Ă��邽�ߎg�p����C���f�b�N�X���Ӂj
	positions[0] = XMVector3Transform(positions[0], m_boneMatrices[ik.nodeIdxes[1]]);

	// �^�񒆂͎����v�Z�����̂Ōv�Z���Ȃ�

	positions[2] = XMVector3Transform(positions[2], m_boneMatrices[ik.boneIdx]);

	// ���[�g�����[�ւ̃x�N�g��������Ă���
	auto linearVec = XMVectorSubtract(positions[2], positions[0]);

	float a = XMVector3Length(linearVec).m128_f32[0];
	float b = edgeLens[0];
	float c = edgeLens[1];

	linearVec = XMVector3Normalize(linearVec);

	// ���[�g����^�񒆂ւ̊p�x�v�Z
	float theta1 = acosf((a * a + b * b - c * c) / (2 * a * b));

	// �^�񒆂���^�[�Q�b�g�ւ̊p�x�v�Z
	float theta2 = acosf((b * b + c * c - a * a) / (2 * b * c));

	// �������߂�
	// �����^�񒆂��u�Ђ��v�ł������ꍇ�ɂ͋����I��X���Ƃ���
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

	// ���ӓ_�FIK�`�F�[���̓��[�g�Ɍ������Ă��琔�����邽��1�����[�g�ɋ߂�
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
	���̊֐��ɗ������_�Ńm�[�h��1�����Ȃ��A
	�`�F�[���ɓ����Ă���m�[�h�ԍ���IK�̃��[�g�m�[�h�̂��̂Ȃ̂ŁA
	���̃��[�g�m�[�h���疖�[�Ɍ������x�N�g�����l����
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
	// �O�ɂ��s�����悤�ɁAIK�I��/�I�t�����t���[���ԍ��ŋt���猟��
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
				// ����IK���I�t�Ȃ�ł��؂�
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
			assert(0); // �Ԃ̃{�[������0�i���肦�Ȃ��j
			continue;

		case 1: // �Ԃ̃{�[������1�̂Ƃ���LookAt
			SolveLookAt(ik);
			break;

		case 2: // �Ԃ̃{�[������2�̂Ƃ��͗]���藝IK
			SolveCosineIK(ik);
			break;

		default: // 3�ȏ�̂Ƃ���CCD-IK
			SolveCCDIK(ik);
		}
	}
}
