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

// PMD�f�[�^
struct PMDHeader
{
	float version;       // ��F00 00 80 3F == 1.00
	char model_name[20]; // ���f����
	char comment[256];   // ���f���R�����g
};

#pragma pack(1)

// PMD���_�f�[�^
struct PMDVertex
{
	XMFLOAT3 pos;             // ���_���W
	XMFLOAT3 normal;          // �@���x�N�g��
	XMFLOAT2 uv;              // uv���W
	unsigned short boneNo[2]; // �{�[���ԍ�
	unsigned char boneWeight; // �{�[���e���x
	unsigned char edgeFlag;   // �֊s���t���O
};

// PMD�}�e���A���\����
struct PMDMaterial
{
	XMFLOAT3 diffuse;        // �f�B�t���[�Y�F
	float alpha;             // �f�B�t���[�Y��
	float specularity;       // �X�y�L�����̋����i��Z�l�j
	XMFLOAT3 specular;       // �X�y�L�����F
	XMFLOAT3 ambient;        // �A���r�G���g�F
	unsigned char toonIdx;   // �g�D�[���ԍ�
	unsigned char edgeFlg;   // �}�e���A�����Ƃ̗֊s���t���O
	unsigned int indicesNum; // ���̃}�e���A�������蓖�Ă���C���f�b�N�X��
	char texFilePath[20];    // �e�N�X�`���t�@�C���p�X+��
};

// �ǂݍ��ݗp�{�[���\����
struct PMDBone
{
	char boneName[20];       // �{�[����
	unsigned short parentNo; // �e�{�[���ԍ�
	unsigned short nextNo;   // ��[�̃{�[���ԍ�
	unsigned char type;      // �{�[�����
	unsigned short ikBoneNo; // IK�{�[���ԍ�
	XMFLOAT3 pos;            // �{�[���̊�_���W
};

#pragma pack()

// �V�F�[�_�[���ɓ�������}�e���A���f�[�^
struct MaterialForHlsl
{
	XMFLOAT3 diffuse;  // �f�B�t���[�Y�F
	float alpha;	   // �f�B�t���[�Y��
	XMFLOAT3 specular; // �X�y�L�����F
	float specularity; // �X�y�L�����̋����i��Z�l�j
	XMFLOAT3 ambient;  // �A���r�G���g�F
};

// ����ȊO�̃}�e���A���f�[�^
struct AdditionalMaterial
{
	std::string texPath; // �e�N�X�`���t�@�C���p�X
	int toonIdx;         // �g�D�[���ԍ�
	bool edgeFlg;        // �}�e���A�����Ƃ̗֊s���t���O
};

// �S�̂��܂Ƃ߂�f�[�^
struct Material
{
	unsigned int indicesNum; // �C���f�b�N�X��
	MaterialForHlsl material;
	AdditionalMaterial additional;
};

struct BoneNode
{
	int boneIdx;					 // �{�[���C���f�b�N�X
	XMFLOAT3 startPos;				 // �{�[����_�i��]�̒��S�j
	XMFLOAT3 endPos;				 // �{�[����[�_�i���ۂ̃X�L�j���O�ɂ͗��p���Ȃ��j
	std::vector<BoneNode*> children; // �q�m�[�h
};

struct VMDMotion
{
	char boneName[15];		  // �{�[����
	unsigned int frameNo;	  // �t���[���ԍ�
	XMFLOAT3 location;		  // �ʒu
	XMFLOAT4 quaternion;	  // �N�H�[�^�j�I���i��]�j
	unsigned char bezier[64]; // [4][4][4]�x�W�F��ԃp�����[�^�[
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
	DWORD m_startTime; // �A�j���[�V�����J�n���̃~���b
};

#endif // !PMD_ACTOR_H_

