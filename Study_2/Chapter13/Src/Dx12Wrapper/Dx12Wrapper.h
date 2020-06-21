#ifndef DX12_WRAPPER_H_
#define DX12_WRAPPER_H_

#include <string>
#include <map>
#include <vector>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <wrl.h>

#include "../Helper/Helper.h"

using namespace DirectX;
using namespace Microsoft::WRL;

using LoadLambda_t = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;

struct SceneMatrix
{
	XMMATRIX view;        // ビュー行列
	XMMATRIX proj;        // プロジェクション行列
	XMMATRIX lightCamera; // ライトから見たビュー
	XMMATRIX shadow;      // 影
	XMFLOAT3 eye;         // 視点座標
};

struct PeraVertex
{
	XMFLOAT3 pos;
	XMFLOAT2 uv;
};

class Dx12Wrapper
{
public:

	Dx12Wrapper();

	ComPtr<ID3D12Device> Device() const;
	ComPtr<ID3D12GraphicsCommandList> CommandList() const;
	const ComPtr<ID3D12Resource> WhiteTex() const;
	const ComPtr<ID3D12Resource> BlackTex() const;
	const ComPtr<ID3D12Resource> GrayGradTex() const;

	void BeginDraw();
	void SetSceneMat();
	void DrawPeraPolygon(bool isToBackBuffer = true);
	void DrawPera2Polygon();
	void EndDraw();
	void WaitForCommandQueue();
	ComPtr<ID3D12Resource> GetTextureByPath(const std::string& path);

private:

	bool InitializeDXGIDevice();
	bool InitializeCommandList();
	bool CreateSwapChain();
	bool CreateFence();
	bool CreateFinalRenderTargets();
	bool CreateDepthStencilView();
	bool CreateSceneView();
	void CreateTextureLoaderTable();
	bool CreateWhiteTexture();
	bool CreateBlackTexture();
	bool CreateGrayGradTexture();
	bool CreatePeraResource();
	bool CreatePeraPolygon();
	bool CreatePeraPipeline();
	bool CreateBokehParamResouece();
	bool CreateEffectBufferAndView();
	bool CreateDepthSRVHeapAndView();
	ComPtr<ID3D12Resource> LoadTextureFromFile(const std::string& texPath);

private:

	ComPtr<ID3D12Device>    m_device{ nullptr };
	ComPtr<IDXGIFactory6>   m_dxgiFactory{ nullptr };
	ComPtr<IDXGISwapChain4> m_swapChain{ nullptr };
	ComPtr<ID3D12CommandAllocator>    m_cmdAllocator{ nullptr };
	ComPtr<ID3D12GraphicsCommandList> m_cmdList{ nullptr };
	ComPtr<ID3D12CommandQueue>        m_cmdQueue{ nullptr };
	ComPtr<ID3D12Fence> m_fence{ nullptr };
	UINT64 m_fenceVal{ 0 };
	ComPtr<ID3D12DescriptorHeap> m_rtvHeaps{ nullptr };
	std::vector<ComPtr<ID3D12Resource>> m_backBuffers;
	ComPtr<ID3D12Resource> m_depthBuffer{ nullptr };
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap{ nullptr };

	ComPtr<ID3D12DescriptorHeap> m_basicDescHeap{ nullptr };
	ComPtr<ID3D12Resource> m_constBuff{ nullptr };
	SceneMatrix* m_mapMat{ nullptr };
	D3D12_VIEWPORT m_viewport{};
	D3D12_RECT m_scissorRect{};
	float m_clearColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	std::map<std::string, LoadLambda_t> m_loadLambdaTable;
	std::map<std::string, ComPtr<ID3D12Resource>> m_resourceTable;
	UINT m_bbIdx;

	ComPtr<ID3D12Resource> m_whiteTex{ nullptr };
	ComPtr<ID3D12Resource> m_blackTex{ nullptr };
	ComPtr<ID3D12Resource> m_gradTex{ nullptr };
	float angle{ 0 };

	ComPtr<ID3D12Resource> m_peraResource{ nullptr };
	ComPtr<ID3D12DescriptorHeap> m_peraRTVHeap{ nullptr }; // レンダーターゲット用
	ComPtr<ID3D12DescriptorHeap> m_peraRegisterHeap{ nullptr }; // テクスチャ用
	ComPtr<ID3D12Resource> m_peraVB{ nullptr };
	D3D12_VERTEX_BUFFER_VIEW m_peraVBV;
	ComPtr<ID3D12RootSignature> m_peraRootSignature{ nullptr };
	ComPtr<ID3D12PipelineState> m_peraPipeline{ nullptr };
	ComPtr<ID3D12Resource> m_bokehParamBuffer{ nullptr };
	ComPtr<ID3D12Resource> m_peraResource2{ nullptr }; // ペラ2枚目
	ComPtr<ID3D12PipelineState> m_pera2Pipeline{ nullptr };

	// 歪みテクスチャ用
	ComPtr<ID3D12DescriptorHeap> m_effectSRVHeap{ nullptr };
	ComPtr<ID3D12Resource> m_effectTexBuffer{ nullptr };

	// 平行ライトの向き
	XMFLOAT3 m_parallelLightVec;
	// 深度値テクスチャ用
	ComPtr<ID3D12DescriptorHeap> m_depthSRVHeap{ nullptr };
};

#endif // !DX12_WRAPPER_H_
