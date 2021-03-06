#include "Dx12Wrapper.h"

#include "../Application/Application.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "DirectXTex.lib")

void EnableDebugLayer()
{
#ifdef _DEBUG

	ID3D12Debug* debugLayer{ nullptr };
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

	debugLayer->EnableDebugLayer(); // デバッグレイヤーを有効化する
	debugLayer->Release(); // 有効化したらインターフェイスを解放する

#endif // _DEBUG
}

Dx12Wrapper::Dx12Wrapper()
{
	EnableDebugLayer();

	if (!InitializeDXGIDevice())
	{
		assert(0);
	}
	if (!InitializeCommandList())
	{
		assert(0);
	}
	if (!CreateSwapChain())
	{
		assert(0);
	}
	if (!CreateFence())
	{
		assert(0);
	}
	if (!CreateFinalRenderTargets())
	{
		assert(0);
	}
	if (!CreateDepthStencilView())
	{
		assert(0);
	}
	if (!CreateSceneView())
	{
		assert(0);
	}
	if (!CreateWhiteTexture())
	{
		assert(0);
	}
	if (!CreateBlackTexture())
	{
		assert(0);
	}
	if (!CreateGrayGradTexture())
	{
		assert(0);
	}

	CreateTextureLoaderTable();
}

// +-------------------------------+ //
// |          public関数           | //
// +-------------------------------+ //

ComPtr<ID3D12Device> Dx12Wrapper::Device() const
{
	return _dev;
}

ComPtr<ID3D12GraphicsCommandList> Dx12Wrapper::CommandList() const
{
	return _cmdList;
}

const ComPtr<ID3D12Resource> Dx12Wrapper::WhiteTex() const
{
	return m_whiteTex;
}

const ComPtr<ID3D12Resource> Dx12Wrapper::BlackTex() const
{
	return m_blackTex;
}

const ComPtr<ID3D12Resource> Dx12Wrapper::GrayGradTex() const
{
	return m_gradTex;
}

void Dx12Wrapper::BeginDraw()
{
	// 現在のバックバッファーのインデックスを取得
    m_bbIdx = _swapChain->GetCurrentBackBufferIndex();

	_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[m_bbIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// レンダーターゲットビュー、デプスステンシルビューをセット
	auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += m_bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();

	_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);

	// レンダーターゲットのクリア
	_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	// 深度のクリア
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0f, 0, nullptr);

	// ビューポートをセット
	_cmdList->RSSetViewports(1, &viewport);

	// シザー矩形をセット
	_cmdList->RSSetScissorRects(1, &scissorRect);
}

void Dx12Wrapper::SetSceneMat()
{
	// ディスクリプタヒープをセット（変換行列用）
	_cmdList->SetDescriptorHeaps(1, basicDescHeap.GetAddressOf());

	// ルートパラメーターとディスクリプタヒープの関連付け（変換行列用）
	_cmdList->SetGraphicsRootDescriptorTable(
		0, // ルートパラメーターインデックス
		basicDescHeap->GetGPUDescriptorHandleForHeapStart()); // ヒープアドレス
}

void Dx12Wrapper::EndDraw()
{
	_cmdList->ResourceBarrier(
		1, &CD3DX12_RESOURCE_BARRIER::Transition(
			_backBuffers[m_bbIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// コマンドリスト実行
	_cmdList->Close();

	ID3D12CommandList* cmdLists[]{ _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, cmdLists);

	WaitForCommandQueue();

	// コマンドリストなどのクリア
	_cmdAllocator->Reset(); // キューのクリア
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);

	// フリップ
	_swapChain->Present(1, 0);
}

void Dx12Wrapper::WaitForCommandQueue()
{
	// 同期処理
	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
	while (_fence->GetCompletedValue() != _fenceVal)
	{
		// イベントハンドルの取得
		auto event = CreateEvent(nullptr, false, false, nullptr);

		_fence->SetEventOnCompletion(_fenceVal, event);

		// イベントが発生するまで待ち続ける（INFINITE）
		WaitForSingleObject(event, INFINITE);

		// イベントハンドルを閉じる
		CloseHandle(event);
	}
}

ComPtr<ID3D12Resource> Dx12Wrapper::GetTextureByPath(const std::string & path)
{
	auto it = _resourceTable.find(path);

	// テーブル内にあったらロードするのではなく
	// マップ内のリソースを返す
	if (it != _resourceTable.end())
	{
		return it->second;
	}

	return LoadTextureFromFile(path);
}

void Dx12Wrapper::UpdateWorldMat()
{
	angle += 0.01f;
	mapMat->world = XMMatrixRotationY(angle);
}

// +-------------------------------+ //
// |          private関数          | //
// +-------------------------------+ //

bool Dx12Wrapper::InitializeDXGIDevice()
{
#ifdef _DEBUG

	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, 
		IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));

#else

	auto result = CreateDXGIFactory1(IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));

#endif // _DEBUG

	if (FAILED(result)) return false;

	std::vector<ComPtr<IDXGIAdapter>> adapters;
	ComPtr<IDXGIAdapter> tmpAdapter{ nullptr };

	for (int i = 0; _dxgiFactory->EnumAdapters(i, tmpAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc{};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;

		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	// Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	//D3D_FEATURE_LEVEL featureLevel;

	for (auto lv : levels)
	{
		// 第1引数をnullptrにするとダメ。
		// Intel(R) UHD Graphics 630でもうまくいかない。モデルが消える
		// 今回は "NVIDIA" がつくアダプターを指定。
		if (D3D12CreateDevice(tmpAdapter.Get(), lv, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())) == S_OK)
		{
			//featureLevel = lv;
			break; // 生成可能なバージョンが見つかったらループを打ち切り
		}
	}

	return true;
}

bool Dx12Wrapper::InitializeCommandList()
{
	// コマンドアロケーター作成
	auto result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// コマンドリスト作成
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
		_cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// コマンドキュー作成
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // フラグなし
	cmdQueueDesc.NodeMask = 0; // アダプターを1つしか使わないときは0でよい
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // プライオリティは特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // コマンドリストと合わせる
	result = _dev->CreateCommandQueue(&cmdQueueDesc,
		IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	return true;
}

bool Dx12Wrapper::CreateSwapChain()
{
	// スワップチェーン作成
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = Application::Instance().WindowWidth();
	swapChainDesc.Height = Application::Instance().WindowHeight();
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH; // バックバッファーは伸び縮み可能
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // フリップ後は速やかに破棄
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // 特に指定なし
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // ウィンドウ⇔フルスクリーン切り替え可能
	auto result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue.Get(), Application::Instance().GetHWND(), &swapChainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)_swapChain.ReleaseAndGetAddressOf());

	if (FAILED(result)) return false;

	return true;
}

bool Dx12Wrapper::CreateFence()
{
	// フェンス作成
	auto result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	return true;
}

bool Dx12Wrapper::CreateFinalRenderTargets()
{
	// RTV作成（まずヒープを作って領域を確保してからRTVを作成）
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビューなのでRTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; // 表裏の2つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // 特に指定なし（シェーダーからどう見えるか）

	auto result = _dev->CreateDescriptorHeap(&heapDesc,
		IID_PPV_ARGS(_rtvHeaps.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	DXGI_SWAP_CHAIN_DESC swcDesc{};
	result = _swapChain->GetDesc(&swcDesc);

	_backBuffers.resize(swcDesc.BufferCount);

	// ビューのアドレス
	auto handle = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	// レンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // ガンマ補正なし（sRGB）
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < swcDesc.BufferCount; ++i)
	{
		// スワップチェーン作成時に確保したバッファを取得
		result = _swapChain->GetBuffer(i, IID_PPV_ARGS(_backBuffers[i].ReleaseAndGetAddressOf()));

		// 取得したバッファのなかにRTVを作成する（っていう流れかな？）
		_dev->CreateRenderTargetView(_backBuffers[i].Get(), &rtvDesc, handle);

		// そのままだと同じ個所をさしてしまうのでアドレスをずらす
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// ビューポートの設定
	viewport.Width = Application::Instance().WindowWidth();   // 出力先の幅（ピクセル数）
	viewport.Height = Application::Instance().WindowHeight(); // 出力先の高さ（ピクセル数）
	viewport.TopLeftX = 0; // 出力先の左上座標 X
	viewport.TopLeftY = 0; // 出力先の左上座標 Y
	viewport.MaxDepth = 1.0f; // 深度最大値
	viewport.MinDepth = 0.0f; // 深度最小値

	scissorRect.top = 0;                // 切り抜き上座標
	scissorRect.left = 0;               // 切り抜き左座標
	scissorRect.right = Application::Instance().WindowWidth();   // 切り抜き右座標
	scissorRect.bottom = Application::Instance().WindowHeight(); // 切り抜きした座標

	return true;
}

bool Dx12Wrapper::CreateDepthStencilView()
{
	// 深度バッファー作成
	D3D12_RESOURCE_DESC depthResDesc{};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元のテクスチャデータ
	depthResDesc.Width = Application::Instance().WindowWidth();   // 幅と高さはレンダーターゲットと同じ
	depthResDesc.Height = Application::Instance().WindowHeight(); // 
	depthResDesc.DepthOrArraySize = 1;   // テクスチャ配列でも、3Dテクスチャでもない
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値書き込み用フォーマット
	depthResDesc.SampleDesc.Count = 1;
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // デプスステンシルとして使用

	// 深度値用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp{};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT; // DEFAULTなのであとはUNKNOWNでよい
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// このクリアバリューが重要な意味を持つ
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f; // 深さ 1.0f(最大値）でクリア
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT; // 32ビットfloat値としてクリア

	auto result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値書き込み
		&depthClearValue,
		IID_PPV_ARGS(depthBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// 深度のためのディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{}; // 深度に使うことがわかればよい
	dsvHeapDesc.NumDescriptors = 1; // 深度ビューは1つ
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // デプスステンシルビューとして使う
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc,
		IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	// 深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値に32ビット使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // フラグは特になし
	_dev->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

bool Dx12Wrapper::CreateSceneView()
{
	// CBV用のヒープを作成
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダーから見えるように
	descHeapDesc.NodeMask = 0; // マスクは0
	descHeapDesc.NumDescriptors = 1; // CBV
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = _dev->CreateDescriptorHeap(&descHeapDesc,
		IID_PPV_ARGS(basicDescHeap.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	XMMATRIX worldMat = XMMatrixIdentity();
	XMFLOAT3 eye{ 0.0f, 10.0f, -30.0f };
	XMFLOAT3 target{ 0.0f, 10.0f, 0.0f };
	XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };

	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,
		static_cast<float>(Application::Instance().WindowWidth())
		/ static_cast<float>(Application::Instance().WindowHeight()),
		1.0f,
		100.0f);

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneMatrix) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(constBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result)) return false;

	result = constBuff->Map(0, nullptr, (void**)&mapMat); // マップ
	mapMat->world = worldMat; // 行列の内容をコピー
	mapMat->view = viewMat;
	mapMat->proj = projMat;
	constBuff->Unmap(0, nullptr);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;

	// 定数バッファビュー作成
	_dev->CreateConstantBufferView(
		&cbvDesc,
		basicDescHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void Dx12Wrapper::CreateTextureLoaderTable()
{
	loadLambdaTable["sph"]
		= loadLambdaTable["spa"]
		= loadLambdaTable["bmp"]
		= loadLambdaTable["png"]
		= loadLambdaTable["jpg"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromWICFile(path.c_str(), 0, meta, img);
	};

	loadLambdaTable["tga"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	loadLambdaTable["dds"]
		= [](const std::wstring& path, TexMetadata* meta, ScratchImage& img)
		->HRESULT
	{
		return LoadFromDDSFile(path.c_str(), 0, meta, img);
	};
}

bool Dx12Wrapper::CreateWhiteTexture()
{
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_whiteTex.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return false;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff); // 全部255で埋める（白）

	result = m_whiteTex->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size());

	if (FAILED(result))
	{
		return false;
	}
	
	return true;
}

bool Dx12Wrapper::CreateBlackTexture()
{
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_blackTex.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return false;
	}

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00); // 全部0で埋める（黒）

	result = m_blackTex->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * 4,
		data.size());

	if (FAILED(result))
	{
		return false;
	}

	return true;
}

bool Dx12Wrapper::CreateGrayGradTexture()
{
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0);

	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_gradTex.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return false;
	}

	// 上が白くて下が黒いテクスチャデータを作成
	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4)
	{
		auto col = (c << 0xff) | (c << 16) | (c << 8) | c;
		std::fill(it, it + 4, col);
		--c;
	}

	result = m_gradTex->WriteToSubresource(
		0,
		nullptr,
		data.data(),
		4 * sizeof(unsigned int),
		sizeof(unsigned int) * data.size());

	if (FAILED(result))
	{
		return false;
	}

	return true;
}

ComPtr<ID3D12Resource> Dx12Wrapper::LoadTextureFromFile(const std::string & texPath)
{
	// WICテクスチャのロード
	TexMetadata metadata{};
	ScratchImage scratchImg{};

	auto wtexPath = GetWideStringFromString(texPath); // テクスチャのファイルパス
	auto ext = GetExtension(texPath);

	if (ext == "")
	{
		return nullptr;
	}

	auto result = loadLambdaTable[ext](
		wtexPath,
		&metadata,
		scratchImg);

	if (FAILED(result))
	{
		return nullptr;
	}

	auto img = scratchImg.GetImage(0, 0, 0); // 生データ抽出

	// WriteToSubresourceで転送する用のヒープ設定
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
	ComPtr<ID3D12Resource> texBuff{ nullptr };
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(texBuff.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		return nullptr;
	}

	result = texBuff->WriteToSubresource(
		0,
		nullptr,
		img->pixels,
		img->rowPitch,
		img->slicePitch);

	if (FAILED(result))
	{
		return nullptr;
	}

	_resourceTable[texPath] = texBuff;

	return texBuff;
}
