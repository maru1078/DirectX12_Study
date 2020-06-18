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
	XMMATRIX world; // モデル本体を回転させたり移動させたりする行列
	XMMATRIX view;  // ビュー行列
	XMMATRIX proj;  // プロジェクション行列
	XMFLOAT3 eye;   // 視点座標
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
	void EndDraw();
	void WaitForCommandQueue();
	ComPtr<ID3D12Resource> GetTextureByPath(const std::string& path);
	void UpdateWorldMat();

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
	ComPtr<ID3D12Resource> LoadTextureFromFile(const std::string& texPath);

private:

	ComPtr<ID3D12Device>    _dev{ nullptr };
	ComPtr<IDXGIFactory6>   _dxgiFactory{ nullptr };
	ComPtr<IDXGISwapChain4> _swapChain{ nullptr };
	ComPtr<ID3D12CommandAllocator>    _cmdAllocator{ nullptr };
	ComPtr<ID3D12GraphicsCommandList> _cmdList{ nullptr };
	ComPtr<ID3D12CommandQueue>        _cmdQueue{ nullptr };
	ComPtr<ID3D12Fence> _fence{ nullptr };
	UINT64 _fenceVal{ 0 };
	ComPtr<ID3D12DescriptorHeap> _rtvHeaps{ nullptr };
	std::vector<ComPtr<ID3D12Resource>> _backBuffers;
	ComPtr<ID3D12Resource> depthBuffer{ nullptr };
	ComPtr<ID3D12DescriptorHeap> dsvHeap{ nullptr };

	ComPtr<ID3D12DescriptorHeap> basicDescHeap{ nullptr };
	ComPtr<ID3D12Resource> constBuff{ nullptr };
	SceneMatrix* mapMat{ nullptr };
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};
	float clearColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	std::map<std::string, LoadLambda_t> loadLambdaTable;
	std::map<std::string, ComPtr<ID3D12Resource>> _resourceTable;
	UINT m_bbIdx;

	ComPtr<ID3D12Resource> m_whiteTex;
	ComPtr<ID3D12Resource> m_blackTex;
	ComPtr<ID3D12Resource> m_gradTex;

	// リファクタリングのテスト用変数
	float angle{ 0 };
};

#endif // !DX12_WRAPPER_H_
