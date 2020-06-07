#pragma once

#include<d3d12.h>
#include<d3dx12.h>
#include<dxgi1_6.h>
#include<map>
#include<unordered_map>
#include<DirectXTex.h>
#include<wrl.h>
#include<string>
#include<functional>
#include <array>

using namespace DirectX;
using namespace Microsoft::WRL;

class Dx12Wrapper
{
	SIZE m_windowSize;

	// DXGI����
	ComPtr<IDXGIFactory6> m_factory{ nullptr };
	ComPtr<IDXGISwapChain4> m_swapChain{ nullptr };

	// DirectX12����
	ComPtr<ID3D12Device> m_device{ nullptr };
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator{ nullptr };
	ComPtr<ID3D12GraphicsCommandList> m_cmdList{ nullptr };
	ComPtr<ID3D12CommandQueue> m_cmdQueue{ nullptr };

	ComPtr<ID3D12Resource> m_depthBuffer{ nullptr };
	std::vector<ID3D12Resource*> m_backBuffers; // �o�b�N�o�b�t�@
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap{ nullptr };
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap{ nullptr };
	std::unique_ptr<CD3DX12_VIEWPORT> m_viewport; // �r���[�|�[�g
	std::unique_ptr<CD3DX12_RECT> m_scissorRect; // �V�U�[��`

	// �V�[�����\������o�b�t�@�܂��
	ComPtr<ID3D12Resource> m_sceneConstBuff{ nullptr };

	// �V�F�[�_�[���ɓn�����߂̊�{�I�Ȋ��f�[�^
	struct SceneData
	{
		XMMATRIX view;   // �r���[�s��
		XMMATRIX proj;   // �v���W�F�N�V�����s��
		XMMATRIX lightCamera; // ���C�g���猩���r���[
		XMMATRIX shadow; // �e
		XMFLOAT3 eye;    // ���_���W
	};

	SceneData* m_mappedSceneData;
	ComPtr<ID3D12DescriptorHeap> m_sceneDescHeap{ nullptr };

	ComPtr<ID3D12Fence> m_fence{ nullptr };
	UINT64 m_fenceVal = 0;

	// ���[�h�p�e�[�u��
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;
	std::map<std::string, LoadLambda_t> m_loadLambdaTable;

	// �e�N�X�`���e�[�u��
	std::unordered_map<std::string, ComPtr<ID3D12Resource>> m_textureTable;

	ComPtr<ID3D12Resource> m_whiteTex{ nullptr };
	ComPtr<ID3D12Resource> m_blackTex{ nullptr };
	ComPtr<ID3D12Resource> m_gradTex{ nullptr };

	// �}���`�p�X�����_�����O
	std::array<ComPtr<ID3D12Resource>, 2> m_pera1Resources;
	ComPtr<ID3D12DescriptorHeap> m_peraRTVHeap; // �����_�[�^�[�Q�b�g�p
	ComPtr<ID3D12DescriptorHeap> m_peraSRVHeap; // �e�N�X�`���p

	ComPtr<ID3D12Resource> m_peraVB;
	D3D12_VERTEX_BUFFER_VIEW m_peraVBV;

	ComPtr<ID3D12RootSignature> m_peraRS;
	ComPtr<ID3D12PipelineState> m_peraPipeline;

	ComPtr<ID3D12Resource> m_bokehParamBuffer;

	// �y���|���S��2����
	ComPtr<ID3D12Resource> m_peraResource2;
	ComPtr<ID3D12PipelineState> m_peraPipeline2;

	// �c�݃e�N�X�`���p
	ComPtr<ID3D12DescriptorHeap> m_effectSRVHeap;
	ComPtr<ID3D12Resource> m_effectTexBuffer;

	// ==========�e�s��p==========

	// ���s���C�g�̌���
	XMFLOAT3 m_parallelLightVec;

	// ==========�V���h�E�}�b�v=========

	// �[�x�l�e�N�X�`���p
	ComPtr<ID3D12DescriptorHeap> m_depthSRVHeap;

	float m_clearColor[4]{ 0.0f, 0.0f, 0.5f, 1.0f };

	XMFLOAT3 m_eye;
	XMFLOAT3 m_target;
	XMFLOAT3 m_up;

	ComPtr<ID3D12Resource> m_lightDepthBuffer;

private:

	// �ŏI�I�ȃ����_�[�^�[�Q�b�g�̐���
	HRESULT CreateFinalRenderTarget();

	// �f�v�X�X�e���V���r���[�̐���
	HRESULT CreateDepthStencilView();

	// �X���b�v�`�F�C���̐���
	HRESULT CreateSwapChain(const HWND& hwnd);

	// DXGI�܂��̏�����
	HRESULT InitializeDXGIDevice();

	// �R�}���h�܂�菉����
	HRESULT InitializeCommand();

	//�r���[�v���W�F�N�V�����p�r���[�̐���
	HRESULT CreateSceneView();

	// �e�N�X�`�����[�_�e�[�u������
	void CreateTextureLoaderTable();

	//�e�N�X�`��������e�N�X�`���o�b�t�@�쐬�A���g���R�s�[
	ID3D12Resource* CreateTextureFromFile(const char* texPath);

	ID3D12Resource* CreateWhiteTexture();
	ID3D12Resource* CreateBlackTexture();
	ID3D12Resource* CreateGrayGradationTexture();

	bool CreatePeraResourceAndView();

	bool CreatePeraPipeline();

	void CreateBokehParamResource();

	bool CreateEffectBufferAndView();

	bool CreateDepthSRV();

public:

	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	void PreDrawShadow();
	void BeginDraw();
	void PreDrawToPera1();
	void SetCameraInfoToConstBuff(); // �J��������萔�o�b�t�@�ɁA�r���[�|�[�g��V�U�[��`�̃Z�b�g
	void SetCameraSetting();
	void DrawHorizontalBokeh();
	void Clear();
	void Draw();
	//void EndDraw();
	void Flip();

	//�e�N�X�`���p�X����K�v�ȃe�N�X�`���o�b�t�@�ւ̃|�C���^��Ԃ�
	//@param texpath �e�N�X�`���t�@�C���p�X
	ComPtr<ID3D12Resource> GetTextureByPath(const char* texPath);

	ComPtr<ID3D12Device> Device();//�f�o�C�X
	ComPtr<ID3D12GraphicsCommandList> CommandList();//�R�}���h���X�g
	ComPtr<IDXGISwapChain4> Swapchain();//�X���b�v�`�F�C��

	ComPtr<ID3D12Resource> WhiteTexture() const;
	ComPtr<ID3D12Resource> BlackTexture() const;
	ComPtr<ID3D12Resource> GradTexture() const;

	//void SetScene();
};