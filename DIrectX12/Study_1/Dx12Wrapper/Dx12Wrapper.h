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

	// DXGI周り
	ComPtr<IDXGIFactory6> m_factory{ nullptr };
	ComPtr<IDXGISwapChain4> m_swapChain{ nullptr };

	// DirectX12周り
	ComPtr<ID3D12Device> m_device{ nullptr };
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator{ nullptr };
	ComPtr<ID3D12GraphicsCommandList> m_cmdList{ nullptr };
	ComPtr<ID3D12CommandQueue> m_cmdQueue{ nullptr };

	ComPtr<ID3D12Resource> m_depthBuffer{ nullptr };
	std::vector<ID3D12Resource*> m_backBuffers; // バックバッファ
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap{ nullptr };
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap{ nullptr };
	std::unique_ptr<CD3DX12_VIEWPORT> m_viewport; // ビューポート
	std::unique_ptr<CD3DX12_RECT> m_scissorRect; // シザー矩形

	// シーンを構成するバッファまわり
	ComPtr<ID3D12Resource> m_sceneConstBuff{ nullptr };

	// シェーダー側に渡すための基本的な環境データ
	struct SceneData
	{
		XMMATRIX view;   // ビュー行列
		XMMATRIX proj;   // プロジェクション行列
		XMMATRIX lightCamera; // ライトから見たビュー
		XMMATRIX shadow; // 影
		XMFLOAT3 eye;    // 視点座標
	};

	SceneData* m_mappedSceneData;
	ComPtr<ID3D12DescriptorHeap> m_sceneDescHeap{ nullptr };

	ComPtr<ID3D12Fence> m_fence{ nullptr };
	UINT64 m_fenceVal = 0;

	// ロード用テーブル
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, TexMetadata*, ScratchImage&)>;
	std::map<std::string, LoadLambda_t> m_loadLambdaTable;

	// テクスチャテーブル
	std::unordered_map<std::string, ComPtr<ID3D12Resource>> m_textureTable;

	ComPtr<ID3D12Resource> m_whiteTex{ nullptr };
	ComPtr<ID3D12Resource> m_blackTex{ nullptr };
	ComPtr<ID3D12Resource> m_gradTex{ nullptr };

	// マルチパスレンダリング
	std::array<ComPtr<ID3D12Resource>, 2> m_pera1Resources;
	ComPtr<ID3D12DescriptorHeap> m_peraRTVHeap; // レンダーターゲット用
	ComPtr<ID3D12DescriptorHeap> m_peraSRVHeap; // テクスチャ用

	ComPtr<ID3D12Resource> m_peraVB;
	D3D12_VERTEX_BUFFER_VIEW m_peraVBV;

	ComPtr<ID3D12RootSignature> m_peraRS;
	ComPtr<ID3D12PipelineState> m_peraPipeline;

	ComPtr<ID3D12Resource> m_bokehParamBuffer;

	// ペラポリゴン2枚目
	ComPtr<ID3D12Resource> m_peraResource2;
	ComPtr<ID3D12PipelineState> m_peraPipeline2;

	// 歪みテクスチャ用
	ComPtr<ID3D12DescriptorHeap> m_effectSRVHeap;
	ComPtr<ID3D12Resource> m_effectTexBuffer;

	// ==========影行列用==========

	// 平行ライトの向き
	XMFLOAT3 m_parallelLightVec;

	// ==========シャドウマップ=========

	// 深度値テクスチャ用
	ComPtr<ID3D12DescriptorHeap> m_depthSRVHeap;

	float m_clearColor[4]{ 0.0f, 0.0f, 0.5f, 1.0f };

	XMFLOAT3 m_eye;
	XMFLOAT3 m_target;
	XMFLOAT3 m_up;

	ComPtr<ID3D12Resource> m_lightDepthBuffer;

private:

	// 最終的なレンダーターゲットの生成
	HRESULT CreateFinalRenderTarget();

	// デプスステンシルビューの生成
	HRESULT CreateDepthStencilView();

	// スワップチェインの生成
	HRESULT CreateSwapChain(const HWND& hwnd);

	// DXGIまわりの初期化
	HRESULT InitializeDXGIDevice();

	// コマンドまわり初期化
	HRESULT InitializeCommand();

	//ビュープロジェクション用ビューの生成
	HRESULT CreateSceneView();

	// テクスチャローダテーブル生成
	void CreateTextureLoaderTable();

	//テクスチャ名からテクスチャバッファ作成、中身をコピー
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
	void SetCameraInfoToConstBuff(); // カメラ情報を定数バッファに、ビューポートやシザー矩形のセット
	void SetCameraSetting();
	void DrawHorizontalBokeh();
	void Clear();
	void Draw();
	//void EndDraw();
	void Flip();

	//テクスチャパスから必要なテクスチャバッファへのポインタを返す
	//@param texpath テクスチャファイルパス
	ComPtr<ID3D12Resource> GetTextureByPath(const char* texPath);

	ComPtr<ID3D12Device> Device();//デバイス
	ComPtr<ID3D12GraphicsCommandList> CommandList();//コマンドリスト
	ComPtr<IDXGISwapChain4> Swapchain();//スワップチェイン

	ComPtr<ID3D12Resource> WhiteTexture() const;
	ComPtr<ID3D12Resource> BlackTexture() const;
	ComPtr<ID3D12Resource> GradTexture() const;

	//void SetScene();
};