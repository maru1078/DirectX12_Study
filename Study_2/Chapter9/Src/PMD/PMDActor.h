#ifndef PMD_ACTOR_H_
#define PMD_ACTOR_H_

#include <memory>
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

class PMDActor
{
public:

	PMDActor(const std::string& filePath, std::weak_ptr<Dx12Wrapper> dx12);

	void Draw();

private:

	bool LoadPMDFile(const std::string& filePath);
	bool CreateMaterialBuffer();
	bool CreateMaterialBufferView();
	bool CreateVertexBuffer();
	bool CreateIndexBuffer();

private:

	std::weak_ptr<Dx12Wrapper> m_dx12;

	std::vector<unsigned char> vertices;
	std::vector<unsigned short> indices;
	std::vector<Material> materials;

	ComPtr<ID3D12Resource> vertBuff{ nullptr };
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	ComPtr<ID3D12Resource> idxBuff{ nullptr };
	D3D12_INDEX_BUFFER_VIEW ibView{};
	ComPtr<ID3D12Resource> materialBuff{ nullptr };

	std::vector<ComPtr<ID3D12Resource>> texResources;
	std::vector<ComPtr<ID3D12Resource>> sphResources;
	std::vector<ComPtr<ID3D12Resource>> spaResources;
	std::vector<ComPtr<ID3D12Resource>> toonResources;

	ComPtr<ID3D12DescriptorHeap> materialDescHeap{ nullptr };
};

#endif // !PMD_ACTOR_H_

