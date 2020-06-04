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

	// ���_�֘A
	ComPtr<ID3D12Resource> m_vb{ nullptr };
	ComPtr<ID3D12Resource> m_ib{ nullptr };
	D3D12_VERTEX_BUFFER_VIEW m_vbView{};
	D3D12_INDEX_BUFFER_VIEW m_ibView{};

	ComPtr<ID3D12Resource> m_transformMat{ nullptr }; // ���W�ϊ��s��i���̓��[���h�̂݁j
	ComPtr<ID3D12DescriptorHeap> m_transformHeap{ nullptr }; // ���W�ϊ��q�[�v

		// �V�F�[�_�[���ɓ�������}�e���A���f�[�^
	struct MaterialForHlsl
	{
		XMFLOAT3 diffuse; // �f�B�t���[�Y
		float alpha; // �f�B�t���[�Y��
		XMFLOAT3 specular; // �X�y�L�����F
		float specularity; // �X�y�L�����F
		XMFLOAT3 ambient; // �A���r�G���g
	};

	// ����ȊO�̃}�e���A���f�[�^
	struct AdditionalMaterial
	{
		std::string texPath; // �e�N�X�`���t�@�C���p�X
		int toonIdx; // �g�D�[���ԍ�
		bool edgeFlag; // �}�e���A�����̗֊s���t���O
	};

	// �܂Ƃ߂�����
	struct Material
	{
		unsigned int indicesNum; // �C���f�b�N�X��
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	struct Transform
	{
		// �����Ɏ����Ă�XMMATRIX�����o��16�o�C�g�A���C�������g�ł��邽��
		// Transform��new����ۂɂ�16�o�C�g���E�Ɋm�ۂ���
		//void* operator new(size_t size);
		XMMATRIX world;
	};

	Transform m_transform;
	Transform* m_mappedTransform{ nullptr };
	ComPtr<ID3D12Resource> m_transformBuffer{ nullptr };

	// �}�e���A���֘A
	std::vector<Material> m_materials;
	ComPtr<ID3D12Resource> m_materialBuff{ nullptr };
	std::vector<ComPtr<ID3D12Resource>> m_textureResources;
	std::vector<ComPtr<ID3D12Resource>> m_sphResources;
	std::vector<ComPtr<ID3D12Resource>> m_spaResources;
	std::vector<ComPtr<ID3D12Resource>> m_toonResources;

	ComPtr<ID3D12DescriptorHeap> m_materialDescHeap{ nullptr };

	// �{�[���֘A
	struct BoneNode
	{
		uint32_t boneIdx; // �{�[���C���f�b�N�X
		uint32_t boneType; // �{�[�����
		uint32_t parentBone; // ��H
		uint32_t ikParentBone; // IK�e�{�[��
		XMFLOAT3 startPos; // �{�[����_�i��]�̒��S�j
		//XMFLOAT3 endPos; // �{�[����[�_�i���ۂ̃X�L�j���O�ɂ͗��p���Ȃ��j
		std::vector<BoneNode*> children; // �q�m�[�h
	};

	std::vector<XMMATRIX> m_boneMatrices;
	std::map<std::string, BoneNode> m_boneNodeTable;
	XMMATRIX* m_mappedMatrices{ nullptr };

	// �C���f�b�N�X���疼�O���������₷���悤��
	std::vector<std::string> m_boneNameArray;

	// �C���f�b�N�X����m�[�h���������₷���悤��
	std::vector<BoneNode*> m_boneNodeAddressArray;

	// ���[�V�����\����
	struct Motion
	{
		unsigned int frameNo; // �A�j���[�V�����J�n����̃t���[����
		XMVECTOR quaternion; // �N�H�[�^�j�I��
		XMFLOAT3 offset; // IK�̏������W����̃I�t�Z�b�g���
		XMFLOAT2 p1, p2; // �x�W�F�Ȑ��̒��ԃR���g���[���|�C���g

		Motion(unsigned int fno, const XMVECTOR& q, XMFLOAT3& ofst, const XMFLOAT2& ip1, const XMFLOAT2& ip2)
			: frameNo(fno), quaternion(q), offset(ofst), p1(ip1), p2(ip2) {}
	};
	
	std::unordered_map<std::string, std::vector<Motion>> m_motionData;
	DWORD m_startTime;
	unsigned int m_duration{ 0 };


	struct PMDIK
	{
		uint16_t boneIdx;                // IK�Ώۂ̃{�[��������
		uint16_t targetIdx;              // �^�[�Q�b�g�ɋ߂Â��邽�߂̃{�[���̃C���f�b�N�X
		uint16_t iterations;             // ���s��
		float limit;                     // 1�񂠂���̉�]����
		std::vector<uint16_t> nodeIdxes; // �Ԃ̃m�[�h�ԍ�
	};

	std::vector<PMDIK> m_pmdIkData;
	std::vector<uint32_t> m_kneeIdxes;

	// IK�I��/�I�t�f�[�^
	struct VMDIKEnable
	{
		// �L�[�t���[��������t���[���ԍ�
		uint32_t frameNo;

		// ���O���I��/�I�t�t���O�̃}�b�v
		std::unordered_map<std::string, bool> ikEnableTable;
	};

	std::vector<VMDIKEnable> m_ikEnableData;

	XMFLOAT3 m_pos{ 0.0f, 0.0f, 0.0f };

	float m_angle; // �e�X�g�pY����]

	unsigned int m_indicesNum;


private:

	// �ǂݍ��񂾃}�e���A�������ƂɃ}�e���A���o�b�t�@���쐬
	HRESULT CreateMaterialData();

	// �}�e���A�����e�N�X�`���r���[�𐶐�
	HRESULT CreateMaterialAndTextureView();

	// ���W�ϊ��p�r���[�̐���
	HRESULT CreateTransformView();

	// PMD�t�@�C���̃��[�h
	HRESULT LoadPMDFile(const char* path);

	void RecursiveMatrixMultiply(BoneNode* node, const XMMATRIX& mat);

	void MotionUpdate();

	float GetYFromXOnBezier(float x, const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n);

	// CCK-IK�ɂ��{�[������������
	// @param ik �Ώ�IK�I�u�W�F�N�g
	void SolveCCDIK(const PMDIK& ik);

	// �]���藝IK�ɂ��{�[������������
	// @param ik �Ώ�IK�I�u�W�F�N�g
	void SolveCosineIK(const PMDIK& ik);

	// LookAt�s��ɂ��{�[������������
	// @param ik �Ώ�IK�I�u�W�F�N�g
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