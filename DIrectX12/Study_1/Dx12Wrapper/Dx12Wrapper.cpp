#include "Dx12Wrapper.h"

#include "../Application/Application.h"
#include "../Helper/Helper.h"

#include <d3dcompiler.h>

#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

HRESULT Dx12Wrapper::CreateFinalRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC1 desc{};
	auto result = m_swapChain->GetDesc1(&desc);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   // レンダーターゲットビューなのでRTV
	heapDesc.NodeMask = 0;                            // GPUの識別のためのビットフラグ（GPUは1つだけなら0固定）
	heapDesc.NumDescriptors = 2;                      // ダブルバッファリング
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // 特に指定なし

	result = m_device->CreateDescriptorHeap(&heapDesc,
		IID_PPV_ARGS(m_rtvHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = m_swapChain->GetDesc(&swcDesc);
	m_backBuffers.resize(swcDesc.BufferCount);

	// 先頭のアドレスを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle
		= m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	// SRGBレンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // ガンマ補正あり（sRGB）
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	// スワップチェーン上のバックバッファを取得
	for (int i = 0; i < swcDesc.BufferCount; ++i)
	{
		result = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));

		rtvDesc.Format = m_backBuffers[i]->GetDesc().Format;

		// RTVの作成
		m_device->CreateRenderTargetView(
			m_backBuffers[i],
			&rtvDesc,
			handle // ディスクリプタヒープハンドル
		);

		// 次へずらす
		handle.ptr += m_device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	m_viewport.reset(new CD3DX12_VIEWPORT(m_backBuffers[0]));
	m_scissorRect.reset(new CD3DX12_RECT(0, 0, desc.Width, desc.Height));

	return result;
}

HRESULT Dx12Wrapper::CreateDepthStencilView()
{
	auto depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32_TYPELESS,
		m_windowSize.cx,
		m_windowSize.cy,
		1,
		1,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	auto depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	auto result = m_device->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値書き込みに使用
		&depthClearValue,
		IID_PPV_ARGS(m_depthBuffer.GetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// 深度のためのディスクリプタヒープを作る
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {}; // 深度に使うことがわかればよい
	dsvHeapDesc.NumDescriptors = 1; // 深度ビューは1つ
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // デプスステンシルビューとして使う
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = m_device->CreateDescriptorHeap(
		&dsvHeapDesc,
		IID_PPV_ARGS(m_dsvHeap.ReleaseAndGetAddressOf())
	);

	// 深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値に32ビット使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // フラグは特になし

	m_device->CreateDepthStencilView(
		m_depthBuffer.Get(),
		&dsvDesc,
		m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	return result;
}

HRESULT Dx12Wrapper::CreateSwapChain(const HWND & hwnd)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

	swapChainDesc.Width = m_windowSize.cx; // 画面解像度【幅】
	swapChainDesc.Height = m_windowSize.cy; // 画面解像度【高】
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // ピクセルフォーマット
	swapChainDesc.Stereo = false; // ステレオ表示フラグ（3Dディスプレイのステレオモード）
	swapChainDesc.SampleDesc.Count = 1; // マルチサンプルの指定（1でよい）
	swapChainDesc.SampleDesc.Quality = 0; // マルチサンプルの指定（0でよい）
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER; // これでよい
	swapChainDesc.BufferCount = 2; // ダブルバッファなら2でよい

	// バックバッファーは伸び縮み可能
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

	// フリップ後は速やかに破棄
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// 特に指定なし
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	// ウィンドウ⇔フルスクリーン切り替え可能
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	return m_factory->CreateSwapChainForHwnd(
		m_cmdQueue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)m_swapChain.ReleaseAndGetAddressOf());
}

HRESULT Dx12Wrapper::InitializeDXGIDevice()
{
	UINT flagsDXGI = 0 | DXGI_CREATE_FACTORY_DEBUG;

	auto result = CreateDXGIFactory2(flagsDXGI, IID_PPV_ARGS(m_factory.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	std::vector<IDXGIAdapter*> adapters; // アダプターの列挙用
	IDXGIAdapter* tmpAdapter{ nullptr };

	for (int i = 0; m_factory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc); // アダプターの説明オブジェクト取得

		std::wstring strDesc = adesc.Description;

		// 探したいアダプターの名前を確認
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	// 生成可能なフィーチャーレベルの検索に使う
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//D3D_FEATURE_LEVEL featureLevel;

	for (auto fl : levels)
	{
		if (SUCCEEDED(D3D12CreateDevice(tmpAdapter, fl, IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()))))
		{
			//featureLevel = fl;
			result = S_OK;
			break; // 生成可能なバージョンが見つかったらループ終了
		}
	}

	return result;
}

HRESULT Dx12Wrapper::InitializeCommand()
{
	auto result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdAllocator));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	result = m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_cmdAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&m_cmdList));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};

	// タイムアウトなし
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	// アダプターを1つしか使わないときは0でよい
	cmdQueueDesc.NodeMask = 0;

	// プライオリティは特に指定なし
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	// コマンドリストと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	result = m_device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&m_cmdQueue));

	return result;
}

HRESULT Dx12Wrapper::CreateSceneView()
{
	DXGI_SWAP_CHAIN_DESC1 desc{};
	auto result = m_swapChain->GetDesc1(&desc);

	// 定数バッファ作成
	result = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_sceneConstBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	m_mappedSceneData = nullptr;
	result = m_sceneConstBuff->Map(0, nullptr, (void**)&m_mappedSceneData);

	SetCameraSetting();

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダーから見えるように
	descHeapDesc.NodeMask = 0; // マスクは 0
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // ヒープ種別

	result = m_device->CreateDescriptorHeap(&descHeapDesc,
		IID_PPV_ARGS(m_sceneDescHeap.ReleaseAndGetAddressOf())); // 生成

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_sceneConstBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_sceneConstBuff->GetDesc().Width;

	// 定数バッファービューの作成
	auto basicHeapHandle = m_sceneDescHeap->GetCPUDescriptorHandleForHeapStart();
	m_device->CreateConstantBufferView(
		&cbvDesc, basicHeapHandle
	);

	return result;
}

void Dx12Wrapper::CreateTextureLoaderTable()
{
	m_loadLambdaTable["sph"] = m_loadLambdaTable["spa"] = m_loadLambdaTable["bmp"] = m_loadLambdaTable["png"] = m_loadLambdaTable["jpg"] = [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT {
		return LoadFromWICFile(path.c_str(), 0, meta, img);
	};


	m_loadLambdaTable["tga"] // TGAなどの一部の 3D ソフトで使用されているテクスチャファイル形式
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	m_loadLambdaTable["dds"] // DirectX用の圧縮テクスチャファイル形式
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromDDSFile(path.c_str(), 0, meta, img);
	};
}

ID3D12Resource * Dx12Wrapper::CreateTextureFromFile(const char * texPath)
{
	// WICテクスチャのロード
	TexMetadata metadata{};
	ScratchImage scratchImg{};

	auto wtexpath = GetWideStringFromString(texPath); // テクスチャのファイルパス

	auto ext = GetExtension(texPath); // 拡張子を取得

	auto result = m_loadLambdaTable[ext](
		wtexpath,
		&metadata,
		scratchImg
		);

	if (FAILED(result))
	{
		return nullptr;
	}

	auto img = scratchImg.GetImage(0, 0, 0); // 生データ抽出

	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		metadata.height,
		metadata.arraySize,
		metadata.mipLevels);

	// バッファ作成
	ID3D12Resource* texBuff{ nullptr };

	result = m_device->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texBuff)
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	result = texBuff->WriteToSubresource(
		0,
		nullptr, // 全領域へコピー
		img->pixels, // 元データアドレス
		img->rowPitch, // 1ラインサイズ
		img->slicePitch // 全サイズ
	);

	if (FAILED(result))
	{
		return nullptr;
	}

	return texBuff;
}

ID3D12Resource * Dx12Wrapper::CreateWhiteTexture()
{
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	ID3D12Resource* whiteBuff{ nullptr };

	auto result = m_device->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteBuff));

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff); // 全部 255 で埋める

	// データ転送
	result = whiteBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size());

	if (FAILED(result))
	{
		return nullptr;
	}

	return whiteBuff;
}

ID3D12Resource * Dx12Wrapper::CreateBlackTexture()
{
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	ID3D12Resource* blackBuff{ nullptr };

	auto result = m_device->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&blackBuff));

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00); // 全部 0 で埋める

	// データ転送
	result = blackBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size());

	if (FAILED(result))
	{
		return nullptr;
	}

	return blackBuff;
}

ID3D12Resource * Dx12Wrapper::CreateGrayGradationTexture()
{
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	ID3D12Resource* gradBuff{ nullptr };

	auto result = m_device->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&gradBuff));

	// 上が白くてしたが黒いテクスチャデータを作成
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;

	for (; it != data.end(); it += 4)
	{
		auto col = (c << 0xff) | (c << 16) | (c << 8) | c;
		std::fill(it, it + 4, col);
		--c;
	}

	// データ転送
	result = gradBuff->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * sizeof(unsigned int),
		sizeof(unsigned int) * data.size());

	if (FAILED(result))
	{
		return nullptr;
	}

	return gradBuff;
}

bool Dx12Wrapper::CreatePeraResourceAndView()
{
	// ==========書き込み先リソースの作成==========

	// 作成済みのヒープ情報を使ってもう1枚作る
	auto heapDesc = m_rtvHeap->GetDesc();

	// 使っているバックバッファーの情報を利用する
	auto& bbuff = m_backBuffers[0];
	auto resDesc = bbuff->GetDesc();

	D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// レンダリング時のクリア値と同じ値
	D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, m_clearColor);

	// 1枚目
	auto result = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(m_peraResource.ReleaseAndGetAddressOf()));

	// 2枚目
	result = m_device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(m_peraResource2.ReleaseAndGetAddressOf()));

	// ==========ビュー（RTV/SRV）を作る==========

	//　RTV用ヒープを作る
	heapDesc.NumDescriptors = 2;
	result = m_device->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(m_peraRTVHeap.ReleaseAndGetAddressOf()));

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	auto handle = m_peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	// レンダーターゲットビュー（RTV）を作る
	m_device->CreateRenderTargetView(
		m_peraResource.Get(),
		&rtvDesc,
		handle);

	handle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_device->CreateRenderTargetView(
		m_peraResource2.Get(),
		&rtvDesc,
		handle);

	// SRV用ヒープを作る
	heapDesc.NumDescriptors = 3; // ガウスパラメーターとシェーダー2つ
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	result = m_device->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(m_peraSRVHeap.ReleaseAndGetAddressOf()));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = rtvDesc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	handle = m_peraSRVHeap->GetCPUDescriptorHandleForHeapStart();

	// シェーダーリソースビュー（SRV）を作る（1枚目）
	m_device->CreateShaderResourceView(
		m_peraResource.Get(),
		&srvDesc,
		handle);

	handle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_device->CreateShaderResourceView(
		m_peraResource2.Get(),
		&srvDesc,
		handle);

	// ==========ボケ用の定数バッファビューを作る==========
	handle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = m_bokehParamBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_bokehParamBuffer->GetDesc().Width;

	m_device->CreateConstantBufferView(
		&cbvDesc, handle);

	return true;
}

bool Dx12Wrapper::CreatePeraPolygon()
{
	struct PeraVertex
	{
		XMFLOAT3 pos;
		XMFLOAT2 uv;
	};

	PeraVertex pv[4] =
	{
		{ { -1, -1, 0.1f }, { 0, 1 } },
		{ { -1,  1, 0.1f }, { 0, 0 } },
		{ {  1, -1, 0.1f }, { 1, 1 } },
		{ {  1,  1, 0.1f }, { 1, 0 } },
	};

	// 頂点バッファーを作成
	auto result = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(pv)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_peraVB.ReleaseAndGetAddressOf()));

	m_peraVBV.BufferLocation = m_peraVB->GetGPUVirtualAddress();
	m_peraVBV.SizeInBytes = sizeof(pv);
	m_peraVBV.StrideInBytes = sizeof(PeraVertex);

	PeraVertex* mappedPera{ nullptr };
	m_peraVB->Map(0, nullptr, (void**)&mappedPera);
	std::copy(std::begin(pv), std::end(pv), mappedPera);
	m_peraVB->Unmap(0, nullptr);

	D3D12_INPUT_ELEMENT_DESC layout[2] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	ComPtr<ID3DBlob> vs;
	ComPtr<ID3DBlob> ps;
	ComPtr<ID3DBlob> errBlob;

	result = D3DCompileFromFile(L"peraVertex.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "vs", "vs_5_0", 0, 0,
		vs.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());

	result = D3DCompileFromFile(L"peraPixel.hlsl", nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, "ps", "ps_5_0", 0, 0,
		ps.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());

	D3D12_DESCRIPTOR_RANGE range[4]{};
	range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[0].BaseShaderRegister = 0;
	range[0].NumDescriptors = 1;

	range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range[1].BaseShaderRegister = 0;
	range[1].NumDescriptors = 1;

	range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[2].BaseShaderRegister = 1;
	range[2].NumDescriptors = 1;

	range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range[3].BaseShaderRegister = 2;
	range[3].NumDescriptors = 1; // 使うのは1個（t2)...2?

	D3D12_ROOT_PARAMETER rp[4]{};
	rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[0].DescriptorTable.pDescriptorRanges = &range[0];
	rp[0].DescriptorTable.NumDescriptorRanges = 1;

	rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[1].DescriptorTable.pDescriptorRanges = &range[1];
	rp[1].DescriptorTable.NumDescriptorRanges = 1;

	rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rp[2].DescriptorTable.pDescriptorRanges = &range[2];
	rp[2].DescriptorTable.NumDescriptorRanges = 1;

	rp[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rp[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rp[3].DescriptorTable.pDescriptorRanges = &range[3];
	rp[3].DescriptorTable.NumDescriptorRanges = 1;

	D3D12_STATIC_SAMPLER_DESC sampler = CD3DX12_STATIC_SAMPLER_DESC(0);
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

	D3D12_ROOT_SIGNATURE_DESC rsDesc{};
	rsDesc.NumParameters = 4;
	rsDesc.pParameters = rp;
	rsDesc.NumStaticSamplers = 1;
	rsDesc.pStaticSamplers = &sampler;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> rsBlob;

	result = D3D12SerializeRootSignature(
		&rsDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		rsBlob.ReleaseAndGetAddressOf(),
		errBlob.ReleaseAndGetAddressOf());

	result = m_device->CreateRootSignature(
		0,
		rsBlob->GetBufferPointer(),
		rsBlob->GetBufferSize(),
		IID_PPV_ARGS(m_peraRS.ReleaseAndGetAddressOf()));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc{};
	gpsDesc.InputLayout.NumElements = _countof(layout);
	gpsDesc.InputLayout.pInputElementDescs = layout;
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	gpsDesc.pRootSignature = m_peraRS.Get();

	// 1枚目用パイプライン生成
	result = m_device->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(m_peraPipeline.ReleaseAndGetAddressOf()));

	// 2枚目用ピクセルシェーダー
	result = D3DCompileFromFile(L"pera.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VerticalBokehPS", "ps_5_0", 0, 0,
		ps.ReleaseAndGetAddressOf(), errBlob.ReleaseAndGetAddressOf());

	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());

	// 2枚目用パイプライン生成
	result = m_device->CreateGraphicsPipelineState(
		&gpsDesc, IID_PPV_ARGS(m_peraPipeline2.ReleaseAndGetAddressOf()));

	return true;
}

void Dx12Wrapper::CreateBokehParamResource()
{
	auto weights = GetGaussianWeights(8, 5.0f);
	auto result = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(
			AlignmentedSize(sizeof(weights[0]) * weights.size(), 256)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_bokehParamBuffer.ReleaseAndGetAddressOf()));

	float* mappedWeight{ nullptr };
	result = m_bokehParamBuffer->Map(0, nullptr, (void**)&mappedWeight);
	copy(weights.begin(), weights.end(), mappedWeight);
	m_bokehParamBuffer->Unmap(0, nullptr);
}

bool Dx12Wrapper::CreateEffectBufferAndView()
{
	// 法線マップをロードする
	m_effectTexBuffer = GetTextureByPath("normal/normalmap.jpg");

	if (m_effectTexBuffer == nullptr)
	{
		return false;
	}

	// ポストエフェクト用のディスクリプタヒープ生成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = m_device->CreateDescriptorHeap(
		&heapDesc, IID_PPV_ARGS(m_effectSRVHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return false;
	}

	// ポストエフェクト用テクスチャビューを作る
	auto desc = m_effectTexBuffer->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = desc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_device->CreateShaderResourceView(
		m_effectTexBuffer.Get(),
		&srvDesc,
		m_effectSRVHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

bool Dx12Wrapper::CreateDepthSRV()
{
	// ヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	
	auto result = m_device->CreateDescriptorHeap(
		&heapDesc, IID_PPV_ARGS(m_depthSRVHeap.ReleaseAndGetAddressOf()));

	// SRV作成
	D3D12_SHADER_RESOURCE_VIEW_DESC resDesc{};
	resDesc.Format = DXGI_FORMAT_R32_FLOAT;
	resDesc.Texture2D.MipLevels = 1;
	resDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	resDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	auto handle = m_depthSRVHeap->GetCPUDescriptorHandleForHeapStart();

	m_device->CreateShaderResourceView(
		m_depthBuffer.Get(), &resDesc, handle);

	return true;
}

Dx12Wrapper::Dx12Wrapper(HWND hwnd)
	: m_eye(0, 15, -25)
	, m_target(0, 10, 0)
	, m_up(0, 1, 0)
{
#ifdef _DEBUG
	//デバッグレイヤーをオンに
	EnableDebugLayer();
#endif

	auto& app = Application::Instance();
	m_windowSize = app.GetWindowSize();
	m_parallelLightVec = { 1.0f, -1.0f, 1.0f };

	//DirectX12関連初期化
	if (FAILED(InitializeDXGIDevice()))
	{
		assert(0);
		return;
	}
	if (FAILED(InitializeCommand()))
	{
		assert(0);
		return;
	}
	if (FAILED(CreateSwapChain(hwnd)))
	{
		assert(0);
		return;
	}
	if (FAILED(CreateFinalRenderTarget()))
	{
		assert(0);
		return;
	}

	if (FAILED(CreateSceneView()))
	{
		assert(0);
		return;
	}

	//テクスチャローダー関連初期化
	CreateTextureLoaderTable();

	//深度バッファ作成
	if (FAILED(CreateDepthStencilView()))
	{
		assert(0);
		return;
	}

	CreateDepthSRV();

	//フェンスの作成
	if (FAILED(m_device->CreateFence(m_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf()))))
	{
		assert(0);
		return;
	}

	m_whiteTex = CreateWhiteTexture();
	m_blackTex = CreateBlackTexture();
	m_gradTex = CreateGrayGradationTexture();

	CreateBokehParamResource();
	CreateEffectBufferAndView();
	CreatePeraResourceAndView();
	CreatePeraPolygon();
}

Dx12Wrapper::~Dx12Wrapper()
{
}

void Dx12Wrapper::Update()
{
}

void Dx12Wrapper::BeginDraw()
{
	auto bbIdx = m_swapChain->GetCurrentBackBufferIndex();

	auto rtvH = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// CD3DX～を使ったバリアの設定
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_backBuffers[bbIdx],
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	auto dsvH = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	m_cmdList->OMSetRenderTargets(
		1,      // レンダーターゲットの数
		&rtvH,  // レンダーターゲットハンドルの先頭アドレス
		false,   // 複数時に連続しているか
		&dsvH // 深度ステンシルバッファービューのハンドル
	);

	// 画面クリア
	m_cmdList->ClearRenderTargetView(
		rtvH,
		m_clearColor,
		0,      // 全画面クリアなら指定する必要なし
		nullptr // 全画面クリアなら指定する必要なし
	);

	m_cmdList->ClearDepthStencilView(
		dsvH,
		D3D12_CLEAR_FLAG_DEPTH,
		1.0f,
		0,
		0,
		nullptr);

	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_peraResource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void Dx12Wrapper::PreDrawToPera1()
{
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_peraResource.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	auto rtvHeapPointer = m_peraRTVHeap->GetCPUDescriptorHandleForHeapStart();
	auto dsvHeapPointer = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	m_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, &dsvHeapPointer);
	m_cmdList->ClearRenderTargetView(rtvHeapPointer, m_clearColor, 0, nullptr);
	m_cmdList->ClearDepthStencilView(dsvHeapPointer, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void Dx12Wrapper::SetCameraInfoToConstBuff()
{
	// ビュー行列とかのセットをしてる。
	// つまり、カメラ情報をセットしてる。

	ID3D12DescriptorHeap* sceneHeaps[] = { m_sceneDescHeap.Get() };

	m_cmdList->SetDescriptorHeaps(1, sceneHeaps);
	m_cmdList->SetGraphicsRootDescriptorTable(0, m_sceneDescHeap->GetGPUDescriptorHandleForHeapStart());

	// ビューポートをセット
	m_cmdList->RSSetViewports(1, m_viewport.get());

	// シザー矩形をセット
	m_cmdList->RSSetScissorRects(1, m_scissorRect.get());
}

void Dx12Wrapper::SetCameraSetting()
{
	auto eyePos = XMLoadFloat3(&m_eye);
	auto targetPos = XMLoadFloat3(&m_target);
	auto upVec = XMLoadFloat3(&m_up);

	m_mappedSceneData->eye = m_eye;
	m_mappedSceneData->view = XMMatrixLookAtLH(eyePos, targetPos, upVec);
	m_mappedSceneData->proj = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,
		static_cast<float>(m_windowSize.cx) / static_cast<float>(m_windowSize.cy),
		0.1f,
		100.0f);

	auto plane = XMFLOAT4(0, 1, 0, 0); // 平面
	XMVECTOR planeVec = XMLoadFloat4(&plane);

	auto light = XMFLOAT4(-1, 1, -1, 0);
	XMVECTOR lightVec = XMLoadFloat4(&light);
	m_mappedSceneData->shadow = XMMatrixShadow(planeVec, lightVec);

	auto lightPos = targetPos + XMVector3Normalize(lightVec)
		* XMVector3Length(XMVectorSubtract(targetPos, eyePos)).m128_f32[0];

	m_mappedSceneData->lightCamera =
		XMMatrixLookAtLH(lightPos, targetPos, upVec)
		* XMMatrixOrthographicLH(40, 40, 0.1f, 100.0f);
}

void Dx12Wrapper::DrawHorizontalBokeh()
{
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_peraResource.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	// 2枚目のリソースの状態をレンダーターゲットに遷移させる
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_peraResource2.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	auto rtvHeapPointer =
		m_peraRTVHeap->GetCPUDescriptorHandleForHeapStart();

	// 2枚目RTVなので進ませてレンダーターゲットを割り当てる
	rtvHeapPointer.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	m_cmdList->OMSetRenderTargets(1, &rtvHeapPointer, false, nullptr);
	m_cmdList->ClearRenderTargetView(rtvHeapPointer, m_clearColor, 0, nullptr);

	m_cmdList->SetGraphicsRootSignature(m_peraRS.Get());
	m_cmdList->SetDescriptorHeaps(1, m_peraSRVHeap.GetAddressOf());

	auto handle = m_peraSRVHeap->GetGPUDescriptorHandleForHeapStart();
	m_cmdList->SetGraphicsRootDescriptorTable(0, handle);

	handle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_cmdList->SetGraphicsRootDescriptorTable(1, handle);

	m_cmdList->SetPipelineState(m_peraPipeline.Get());
	m_cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_cmdList->IASetVertexBuffers(0, 1, &m_peraVBV);
	m_cmdList->DrawInstanced(4, 1, 0, 0);

	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_peraResource2.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void Dx12Wrapper::Clear()
{
	auto bbIdx = m_swapChain->GetCurrentBackBufferIndex();

	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_backBuffers[bbIdx],
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET));

	auto rtvH = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	m_cmdList->OMSetRenderTargets(
		1,      // レンダーターゲットの数
		&rtvH,  // レンダーターゲットハンドルの先頭アドレス
		false,   // 複数時に連続しているか
		nullptr // 深度ステンシルバッファービューのハンドル
	);

	m_cmdList->ClearRenderTargetView(rtvH, m_clearColor, 0, nullptr);
}

void Dx12Wrapper::Draw()
{
	m_cmdList->SetGraphicsRootSignature(m_peraRS.Get());

	// まずヒープをセット
	m_cmdList->SetDescriptorHeaps(1, m_peraSRVHeap.GetAddressOf());
	auto handle = m_peraSRVHeap->GetGPUDescriptorHandleForHeapStart();

	// パラメーター0番とヒープを関連付ける
	m_cmdList->SetGraphicsRootDescriptorTable(0, handle);

	handle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	handle.ptr += m_device->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_cmdList->SetGraphicsRootDescriptorTable(1, handle);

	// 深度バッファテクスチャ
	m_cmdList->SetDescriptorHeaps(1, m_depthSRVHeap.GetAddressOf());
	m_cmdList->SetGraphicsRootDescriptorTable(3, 
		m_depthSRVHeap->GetGPUDescriptorHandleForHeapStart());

	m_cmdList->SetPipelineState(m_peraPipeline.Get());
	m_cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_cmdList->IASetVertexBuffers(0, 1, &m_peraVBV);
	m_cmdList->SetDescriptorHeaps(1, m_effectSRVHeap.GetAddressOf());
	m_cmdList->SetGraphicsRootDescriptorTable(2, 
		m_effectSRVHeap->GetGPUDescriptorHandleForHeapStart());
	m_cmdList->DrawInstanced(4, 1, 0, 0);
}

void Dx12Wrapper::Flip()
{
	m_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			m_backBuffers[m_swapChain->GetCurrentBackBufferIndex()],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));

	// 命令のクローズ
	m_cmdList->Close();

	// コマンドリストの実行
	ID3D12CommandList* cmdLists[] = { m_cmdList.Get() };
	m_cmdQueue->ExecuteCommandLists(
		1,       // 実行するコマンドリストの数
		cmdLists // コマンドリストの配列の先頭アドレス
	);

	m_cmdQueue->Signal(m_fence.Get(), ++m_fenceVal);

	if (m_fence->GetCompletedValue() < m_fenceVal)
	{
		// イベントハンドルの取得
		auto event = CreateEvent(nullptr, false, false, nullptr);

		m_fence->SetEventOnCompletion(m_fenceVal, event);

		// イベントが発生するまで待ち続ける（INFINITE）
		WaitForSingleObject(event, INFINITE);

		// イベントハンドルを閉じる
		CloseHandle(event);
	}

	m_cmdAllocator->Reset(); // キューをクリア（なぜ「キュー」のクリア？）
	m_cmdList->Reset(m_cmdAllocator.Get(), nullptr); // 再びコマンドリストをためる準備
	m_swapChain->Present(0, 0);
}

ComPtr<ID3D12Resource> Dx12Wrapper::GetTextureByPath(const char * texPath)
{
	auto it = m_textureTable.find(texPath);

	if (it != m_textureTable.end())
	{
		// テーブル内にあったらロードするのではなく
		// マップ内のリソースを返す
		return m_textureTable[texPath];
	}
	else
	{
		return ComPtr<ID3D12Resource>(CreateTextureFromFile(texPath));
	}
}

ComPtr<ID3D12Device> Dx12Wrapper::Device()
{
	return m_device;
}

ComPtr<ID3D12GraphicsCommandList> Dx12Wrapper::CommandList()
{
	return m_cmdList;
}

ComPtr<IDXGISwapChain4> Dx12Wrapper::Swapchain()
{
	return m_swapChain;
}

ComPtr<ID3D12Resource> Dx12Wrapper::WhiteTexture() const
{
	return m_whiteTex;
}

ComPtr<ID3D12Resource> Dx12Wrapper::BlackTexture() const
{
	return m_blackTex;
}

ComPtr<ID3D12Resource> Dx12Wrapper::GradTexture() const
{
	return m_gradTex;
}
