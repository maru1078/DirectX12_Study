#include <Windows.h>
#include <tchar.h>

#ifdef _DEBUG
#include <iostream>
#endif

using namespace std;

#include <d3d12.h>
#include <dxgi1_6.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <wrl.h>
using namespace Microsoft::WRL;

#include <vector>

#include <DirectXMath.h>
using namespace DirectX;

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#include <DirectXTex.h>
#pragma comment(lib, "DirectXTex.lib")

#include <d3dx12.h>

constexpr size_t pmdvertex_size = 38; // 1頂点当たりのサイズ

// @brief コンソール画面にフォーマット付き文字列を表示
// @param format フォーマット（%dとか%fとかの）
// @param 可変長引数
// @remarks この関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG

	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);

#endif
}

// アラインメントにそろえたサイズを返す
// @param size 元のサイズ
// @param alignment アラインメントサイズ
// @return アラインメントをそろえたサイズ
size_t AlignmentedSize(size_t size, size_t alignment)
{
	if (size % alignment == 0) return size;

	return size + alignment - size % alignment;
}

void EnableDebugLayer()
{
	ID3D12Debug* debugLayer{ nullptr };
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

	debugLayer->EnableDebugLayer(); // デバッグレイヤーを有効化する
	debugLayer->Release(); // 有効化したらインターフェイスを解放する
}

const int window_width = 960;
const int window_height = 540;

// 面倒だけど書かなければいけない関数
HRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0); // OSに対して「もうこのアプリは終わる」と伝える
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam); // 既定の処理を行う
}

#ifdef _DEBUG

int main()
{

#else

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

#endif

	DebugOutputFormatString("Show window test.");

	// ウィンドウクラスの生成＆登録
	WNDCLASSEX w{};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure; // コールバック関数の指定
	w.lpszClassName = _T("DX12Sample");       // アプリケーションクラス名（適当でよい）
	w.hInstance = GetModuleHandle(nullptr);   // ハンドルの取得

	RegisterClassEx(&w); // アプリケーションクラス（ウィンドウクラスの指定をOSに伝える）

	RECT wrc = { 0, 0, window_width, window_height }; // ウィンドウサイズを決める

	// 関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(
		w.lpszClassName,      // クラス名指定
		_T("DX12テスト"),	  // タイトルバーの文字
		WS_OVERLAPPEDWINDOW,  // タイトルバーと境界があるウィンドウ
		CW_USEDEFAULT,		  // 表示 X 座標はOSにお任せ
		CW_USEDEFAULT,		  // 表示 Y 座標はOSにお任せ
		wrc.right - wrc.left, // ウィンドウ幅
		wrc.bottom - wrc.top, // ウィンドウ高
		nullptr,			  // 親ウィンドウハンドル
		nullptr,			  // メニューハンドル
		w.hInstance,		  // 呼び出しアプリケーションハンドル
		nullptr);			  // 追加パラメーター

	// ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	// ==========DirectX12関係==========

#ifdef _DEBUG

	// デバッグレイヤーをオンに
	EnableDebugLayer();

#endif

	ComPtr<ID3D12Device>    _dev{ nullptr };
	ComPtr<IDXGIFactory6>   _dxgiFactory{ nullptr };
	ComPtr<IDXGISwapChain4> _swapChain{ nullptr };

	// Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D_FEATURE_LEVEL featureLevel;

	for (auto lv : levels)
	{
		if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())))
		{
			featureLevel = lv;
			break; // 生成可能なバージョンが見つかったらループを打ち切り
		}
	}

	// ファクトリーの作成
#ifdef _DEBUG

	// DXGI関係のエラーメッセージを表示できる
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));

#else

	auto result = CreateDXGIFactory1(IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));

#endif
	ComPtr<ID3D12CommandAllocator>    _cmdAllocator{ nullptr };
	ComPtr<ID3D12GraphicsCommandList> _cmdList{ nullptr };
	ComPtr<ID3D12CommandQueue>        _cmdQueue{ nullptr };

	// コマンドアロケーター作成
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));

	// コマンドリスト作成
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));

	// コマンドキュー作成
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc{};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE; // フラグなし
	cmdQueueDesc.NodeMask = 0; // アダプターを1つしか使わないときは0でよい
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // プライオリティは特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // コマンドリストと合わせる
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));

	// スワップチェーン作成
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = window_width;
	swapChainDesc.Height = window_height;
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
	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)_swapChain.ReleaseAndGetAddressOf());

	// フェンス作成
	ComPtr<ID3D12Fence> _fence{ nullptr };
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf()));

	// RTV作成（まずヒープを作って領域を確保してからRTVを作成）
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビューなのでRTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; // 表裏の2つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // 特に指定なし（シェーダーからどう見えるか。今回はシェーダーから見ることはないので指定なし）

	ComPtr<ID3D12DescriptorHeap> _rtvHeaps{ nullptr };
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_rtvHeaps.ReleaseAndGetAddressOf()));

	DXGI_SWAP_CHAIN_DESC swcDesc{};
	result = _swapChain->GetDesc(&swcDesc);

	std::vector<ComPtr<ID3D12Resource>> _backBuffers(swcDesc.BufferCount);

	// ビューのアドレス
	auto handle = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	// SRGBレンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // ガンマ補正あり（sRGB）
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

	XMMATRIX worldMat = XMMatrixIdentity();
	XMFLOAT3 eye{ 0.0f, 10.0f, -15.0f };
	XMFLOAT3 target{ 0.0f, 10.0f, 0.0f };
	XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };

	worldMat = XMMatrixRotationY(XM_PIDIV4);
	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,
		static_cast<float>(window_width) / static_cast<float>(window_height),
		1.0f,
		100.0f);

	// 定数バッファ作成
	ComPtr<ID3D12Resource> constBuff{ nullptr };

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(worldMat) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(constBuff.ReleaseAndGetAddressOf()));

	XMMATRIX* mapMat{ nullptr }; // マップ先を示すポインター
	result = constBuff->Map(0, nullptr, (void**)&mapMat); // マップ
	*mapMat = worldMat; // 行列の内容をコピー

		// PMDデータ
	struct PMDHeader
	{
		float version;       // 例：00 00 80 3F == 1.00
		char model_name[20]; // モデル名
		char comment[256];   // モデルコメント
	};

	// PMD頂点データ
	struct PMDVertex
	{
		XMFLOAT3 pos;             // 頂点座標
		XMFLOAT3 normal;          // 法線ベクトル
		XMFLOAT2 uv;              // uv座標
		unsigned short boneNo[2]; // ボーン番号
		unsigned char boneWeight; // ボーン影響度
		unsigned char edgeFlag;   // 輪郭線フラグ
	};

	PMDHeader pmdHeader;
	char signature[3]{}; // pmdファイルの先頭3バイトが「Pmd」となっているため、それは構造体に含めない
	FILE* fp;
	unsigned int vertNum; // 頂点数
	std::vector<unsigned char> vertices; // バッファの確保
	unsigned int indicesNum; // インデックス数
	std::vector<unsigned short> indices;
	
	fopen_s(&fp, "Model/初音ミク.pmd", "rb");
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdHeader, sizeof(pmdHeader), 1, fp);

	fread(&vertNum, sizeof(vertNum), 1, fp);
	vertices.resize(vertNum * pmdvertex_size);
	fread(vertices.data(), vertices.size(), 1, fp);
	
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	fclose(fp);

	struct Vertex
	{
		XMFLOAT3 pos; // xyz座標
		XMFLOAT2 uv;  // uv座標
	};

	//// 頂点バッファ作成
	//Vertex vertices[4] =
	//{
	//	{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } }, // 左下
	//	{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f } }, // 左上
	//	{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } }, // 右下
	//	{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f } }, // 右上
	//};

	//D3D12_HEAP_PROPERTIES heapProp{};
	//heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	//heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	//heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	//D3D12_RESOURCE_DESC resDesc{};
	//resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	//resDesc.Width = sizeof(vertices); // 頂点情報が入るだけのサイズ
	//resDesc.Height = 1;
	//resDesc.DepthOrArraySize = 1;
	//resDesc.MipLevels = 1;
	//resDesc.Format = DXGI_FORMAT_UNKNOWN;
	//resDesc.SampleDesc.Count = 1;
	//resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	//resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ComPtr<ID3D12Resource> vertBuff{ nullptr };

	//result = _dev->CreateCommittedResource(
	//	&heapProp,
	//	D3D12_HEAP_FLAG_NONE,
	//	&resDesc,
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	nullptr,
	//	IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf()));

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertices.size()), // サイズ変更
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(vertBuff.ReleaseAndGetAddressOf()));

	unsigned char* vertMap{ nullptr };
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView{};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress(); // バッファの仮想アドレス
	vbView.SizeInBytes = vertices.size(); // 全バイト数
	vbView.StrideInBytes = pmdvertex_size; // 1頂点あたりのバイト数

	// インデックスバッファ作成
	//unsigned short indices[] =
	//{
	//	0, 1, 2,
	//	2, 1, 3
	//};

	ComPtr<ID3D12Resource> idxBuff{ nullptr };

	//// 設定は、バッファのサイズ以外、頂点バッファの設定を使いまわしてよい
	//resDesc.Width = sizeof(indices);

	//result = _dev->CreateCommittedResource(
	//	&heapProp,
	//	D3D12_HEAP_FLAG_NONE,
	//	&resDesc,
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	nullptr,
	//	IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf()));

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(idxBuff.ReleaseAndGetAddressOf()));

	D3D12_INDEX_BUFFER_VIEW ibView{};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);

	// 作ったバッファーにインデックスデータをコピー
	unsigned short* mappedIdx{ nullptr };
	result = idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(std::begin(indices), std::end(indices), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	ComPtr<ID3DBlob> vsBlob{ nullptr };
	ComPtr<ID3DBlob> psBlob{ nullptr };
	ComPtr<ID3DBlob> errorBlob{ nullptr };

	// 頂点シェーダー
	result = D3DCompileFromFile(
		L"Shader/BasicVertexShader.hlsl", // シェーダー名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルードはデフォルト
		"BasicVS", // 関数は「BasicVS」
		"vs_5_0", // 対象シェーダーは「vs_5_0」
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用及び最適化はなし
		0,
		vsBlob.ReleaseAndGetAddressOf(),
		errorBlob.ReleaseAndGetAddressOf()); // エラー時はメッセージが入る

	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見当たりません");
			return 0; // exit()などに適宜置き換える方がよい
		}
		else
		{
			std::string errStr;
			errStr.resize(errorBlob->GetBufferSize());

			std::copy_n(
				(char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errStr.begin());
			errStr += "\n";
			::OutputDebugStringA(errStr.c_str());
		}
	}

	// ピクセルシェーダー
	result = D3DCompileFromFile(
		L"Shader/BasicPixelShader.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		psBlob.ReleaseAndGetAddressOf(),
		errorBlob.ReleaseAndGetAddressOf());

	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見当たりません");
			return 0; // exit()などに適宜置き換える方がよい
		}
		else
		{
			std::string errStr;
			errStr.resize(errorBlob->GetBufferSize());

			std::copy_n(
				(char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errStr.begin());
			errStr += "\n";
			::OutputDebugStringA(errStr.c_str());
		}
	}

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // 座標
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // 法線
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // uv
		{ "BONE_NO",  0, DXGI_FORMAT_R16G16_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // ボーン番号
		{ "WEIGHT",   0, DXGI_FORMAT_R8_UINT,         0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, // ウェイト
	};

	//// テクスチャデータの作成
	//struct TexRGBA
	//{
	//	unsigned char R, G, B, A;
	//};

	//std::vector<TexRGBA> textureData(256 * 256);
	//for (auto& rgba : textureData)
	//{
	//	rgba.R = rand() % 256;
	//	rgba.G = rand() % 256;
	//	rgba.B = rand() % 256;
	//	rgba.A = 255; // αは1.0fとする
	//}

	//// WICテクスチャのロード
	//TexMetadata metaData{};
	//ScratchImage scratchImg{};

	//result = LoadFromWICFile(
	//	L"Image/textest.png", WIC_FLAGS_NONE,
	//	&metaData, scratchImg);

	//auto img = scratchImg.GetImage(0, 0, 0);

	//// 公式で推奨されている方法
	//// 中間バッファとしてのアップロードヒープ設定
	//D3D12_HEAP_PROPERTIES uploadHeapProp{};
	//uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD; // マップ可能にするため、UPLOADにする
	//uploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN; // アップロード用に使用すること前提なのでUNKNOWNでよい
	//uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	//uploadHeapProp.CreationNodeMask = 0; // 単一アダプタのため 0
	//uploadHeapProp.VisibleNodeMask = 0;  // 単一アダプタのため 0

	//ComPtr<ID3D12Resource> uploadBuff{ nullptr };

	{
		//D3D12_RESOURCE_DESC resDesc{};
		//resDesc.Format = DXGI_FORMAT_UNKNOWN; // 単なるデータの塊なので UNKNOWN
		//resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // 単なるバッファーとして指定
		//resDesc.Width = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height; // データサイズ
		//resDesc.Height = 1;
		//resDesc.DepthOrArraySize = 1;
		//resDesc.MipLevels = 1;
		//resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // 連続したデータ
		//resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // 特に指定なし
		//resDesc.SampleDesc.Count = 1; // 通常テクスチャなのでアンチエイリアシングしない
		//resDesc.SampleDesc.Quality = 0;
		//result = _dev->CreateCommittedResource(
		//	&uploadHeapProp,
		//	D3D12_HEAP_FLAG_NONE, // 特に指定なし
		//	&resDesc,
		//	D3D12_RESOURCE_STATE_GENERIC_READ,
		//	nullptr,
		//	IID_PPV_ARGS(uploadBuff.ReleaseAndGetAddressOf()));
	}

	//ComPtr<ID3D12Resource> texBuff{ nullptr };

	// 上でheapPropを使ってるので無理やり。（簡単な方）
	{
		//// WriteToSubresourceで転送するためのヒープ設定
		//D3D12_HEAP_PROPERTIES heapProp{};

		//// 特殊な設定なのでDEFAULTでもUPLOADでもない
		//heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;

		//// ライトバック
		//heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;

		//// 転送はL0、つまりCPU側から直接行う
		//heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

		//// 単一アダプターのため 0
		//heapProp.CreationNodeMask = 0;
		//heapProp.VisibleNodeMask = 0;

		//D3D12_RESOURCE_DESC resDesc{};
		//resDesc.Format = metaData.format; // RGBAフォーマット
		//resDesc.Width = metaData.width; // 幅
		//resDesc.Height = metaData.height; // 高さ
		//resDesc.DepthOrArraySize = metaData.arraySize; // 2Dで配列でもないので 1
		//resDesc.SampleDesc.Count = 1; // 通常テクスチャなのでアンチエイリアシングしない
		//resDesc.SampleDesc.Quality = 0; // クオリティは最低
		//resDesc.MipLevels = metaData.mipLevels;          // ミップマップしないのでミップ数は1つ
		//resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData.dimension); // 2Dテクスチャ
		//resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // レイアウトは決定しない
		//resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // 特に指定なし

		//result = _dev->CreateCommittedResource(
		//	&heapProp,
		//	D3D12_HEAP_FLAG_NONE, // 特に指定なし
		//	&resDesc,
		//	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // シェーダーリソースとして作成
		//	nullptr,
		//	IID_PPV_ARGS(texBuff.ReleaseAndGetAddressOf()));

		//result = texBuff->WriteToSubresource(
		//	0,
		//	nullptr, // 全領域コピー
		//	img->pixels, // 元データアドレス
		//	img->rowPitch, // 1ラインサイズ
		//	img->slicePitch); // 全サイズ
	}

	// 公式で推奨されてるやり方（難しい）
	{
		//// もう一つのやり方用のヒープ設定
		//D3D12_HEAP_PROPERTIES texHeapProp{};
		//texHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT; // テクスチャ用
		//texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		//texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		//texHeapProp.CreationNodeMask = 0;
		//texHeapProp.VisibleNodeMask = 0;

		//D3D12_RESOURCE_DESC resDesc{};
		//resDesc.Format = metaData.format; // RGBAフォーマット
		//resDesc.Width = metaData.width; // 幅
		//resDesc.Height = metaData.height; // 高さ
		//resDesc.DepthOrArraySize = metaData.arraySize; // 2Dで配列でもないので 1
		//resDesc.SampleDesc.Count = 1; // 通常テクスチャなのでアンチエイリアシングしない
		//resDesc.SampleDesc.Quality = 0; // クオリティは最低
		//resDesc.MipLevels = metaData.mipLevels;          // ミップマップしないのでミップ数は1つ
		//resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData.dimension); // 2Dテクスチャ
		//resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // レイアウトは決定しない
		//resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // 特に指定なし

		//result = _dev->CreateCommittedResource(
		//	&texHeapProp,
		//	D3D12_HEAP_FLAG_NONE, // 特に指定なし
		//	&resDesc,
		//	D3D12_RESOURCE_STATE_COPY_DEST, // コピー先
		//	nullptr,
		//	IID_PPV_ARGS(texBuff.ReleaseAndGetAddressOf()));

		//decltype(img->pixels) mapforImg = nullptr;
		//result = uploadBuff->Map(0, nullptr, (void**)&mapforImg);
		////std::copy_n(img->pixels, img->slicePitch, mapforImg);

		//auto srcAddress = img->pixels;
		//auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);

		//for (int y = 0; y < img->height; ++y)
		//{
		//	std::copy_n(srcAddress, rowPitch, mapforImg); // コピー

		//	// 1行ごとにつじつまを合わせる
		//	srcAddress += img->rowPitch;
		//	mapforImg += rowPitch;
		//}

		//uploadBuff->Unmap(0, nullptr);

		//D3D12_TEXTURE_COPY_LOCATION src{};
		//// コピー元（アップロード側）設定
		//src.pResource = uploadBuff.Get(); // 中間バッファ
		//src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; // フットプリント指定
		//src.PlacedFootprint.Offset = 0;
		//src.PlacedFootprint.Footprint.Width = metaData.width;
		//src.PlacedFootprint.Footprint.Height = metaData.height;
		//src.PlacedFootprint.Footprint.Depth = metaData.depth;
		//src.PlacedFootprint.Footprint.RowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		//src.PlacedFootprint.Footprint.Format = img->format;

		//D3D12_TEXTURE_COPY_LOCATION dst{};
		//// コピー先設定
		//dst.pResource = texBuff.Get();
		//dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		//dst.SubresourceIndex = 0;

		//_cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

		//D3D12_RESOURCE_BARRIER barrierDesc{};
		//barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		//barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		//barrierDesc.Transition.pResource = texBuff.Get();
		//barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		//barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST; // 重要
		//barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // 重要

		//_cmdList->ResourceBarrier(1, &barrierDesc);
		//_cmdList->Close();

		//// コマンドリストの実行
		//ID3D12CommandList* cmdLists[]{ _cmdList.Get() };
		//_cmdQueue->ExecuteCommandLists(1, cmdLists);

		//// 同期処理
		//_cmdQueue->Signal(_fence.Get(), ++_fenceVal);
		//if (_fence->GetCompletedValue() != _fenceVal)
		//{
		//	// イベントハンドルの取得
		//	auto event = CreateEvent(nullptr, false, false, nullptr);

		//	_fence->SetEventOnCompletion(_fenceVal, event);

		//	// イベントが発生するまで待ち続ける（INFINITE）
		//	WaitForSingleObject(event, INFINITE);

		//	// イベントハンドルを閉じる
		//	CloseHandle(event);
		//}
		//_cmdAllocator->Reset(); // キューのクリア
		//_cmdList->Reset(_cmdAllocator.Get(), nullptr); // 再びコマンドリストをためる準備
	}

	ComPtr<ID3D12DescriptorHeap> basicDescHeap{ nullptr };
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc{};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダーから見えるように
	descHeapDesc.NodeMask = 0; // マスクは0
	descHeapDesc.NumDescriptors = 2; // SRV, CBV
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // シェーダーリソースビュー用
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(basicDescHeap.ReleaseAndGetAddressOf()));

	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	//srvDesc.Format = metaData.format; // RGBA（0.0f～1.0fに正規化）
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	//srvDesc.Texture2D.MipLevels = 1; // ミップマップは使用しないので 1

	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();

	//_dev->CreateShaderResourceView(
	//	texBuff.Get(), // ビューと関連付けるバッファ
	//	&srvDesc, // 先ほど設定したテクスチャ設定情報
	//	basicHeapHandle); // ヒープのどこに割り当てるか

	//basicHeapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;

	// 定数バッファビュー作成
	_dev->CreateConstantBufferView(
		&cbvDesc,
		basicHeapHandle);

	// レンジの作成
	D3D12_DESCRIPTOR_RANGE range{};
	
	//// テクスチャ用
	//range[0].NumDescriptors = 1; // テクスチャ1つ
	//range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // 種別はテクスチャ
	//range[0].BaseShaderRegister = 0; // 0番スロットから
	//range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 定数用
	range.NumDescriptors = 1;
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	range.BaseShaderRegister = 0;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメーター作成
	D3D12_ROOT_PARAMETER rootParam{};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // シェーダーから見える
	rootParam.DescriptorTable.pDescriptorRanges = &range; // ディスクリプタレンジのアドレス
	rootParam.DescriptorTable.NumDescriptorRanges = 1; // レンジ数

	// サンプラー作成
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 横方向の繰り返し
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 縦方向の繰り返し
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 奥行きの繰り返し
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK; // ボーダーは黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // 線形補間
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX; // ミップマップ最大値
	samplerDesc.MinLOD = 0.0f; // ミップマップ最小値
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダーから見える
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // リサンプリングしない

	// ルートシグネチャ作成
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = &rootParam; // ルートパラメーターの先頭アドレス
	rootSignatureDesc.NumParameters = 1; // ルートパラメーター数
	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	ID3DBlob* rootSigBlob{ nullptr };
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc, // ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0, // ルートシグネチャバージョン
		&rootSigBlob, // シェーダーを作った時と同じ
		errorBlob.ReleaseAndGetAddressOf());

	ComPtr<ID3D12RootSignature> rootSignature{ nullptr };

	result = _dev->CreateRootSignature(
		0, // nodemask。0でよい
		rootSigBlob->GetBufferPointer(), // シェーダーのときと同じ
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(rootSignature.ReleaseAndGetAddressOf()));

	rootSigBlob->Release(); // 不要になったので解放

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline{};

	gpipeline.pRootSignature = rootSignature.Get();

	// 頂点シェーダー設定
	gpipeline.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = vsBlob->GetBufferSize();

	// ピクセルシェーダー設定
	gpipeline.PS.pShaderBytecode = psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = psBlob->GetBufferSize();

	// デフォルトのサンプルマスクを表す定数（0xffffffff）
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// ラスタライザー設定
	gpipeline.RasterizerState.MultisampleEnable = false; // まだアンチエイリアシングを使わないため false
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // 中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true; // 深度方向のクリッピングは有効に

	// ブレンドステート設定
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc{};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;
	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	gpipeline.InputLayout.pInputElementDescs = inputLayout; // レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout); // レイアウト配列の要素数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // カットなし

	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 三角形で構成

	// レンダーターゲット設定
	gpipeline.NumRenderTargets = 1;
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 0～1に正規化されたRGBA

	// サンプル数指定
	gpipeline.SampleDesc.Count = 1; // サンプリングは 1 ピクセルにつき 1
	gpipeline.SampleDesc.Quality = 0; // クオリティは最低

	// パイプライン作成
	ComPtr<ID3D12PipelineState> _pipelineState{ nullptr };
	result = _dev->CreateGraphicsPipelineState(
		&gpipeline,
		IID_PPV_ARGS(_pipelineState.ReleaseAndGetAddressOf()));

	// ビューポート
	D3D12_VIEWPORT viewport{};
	viewport.Width = window_width;   // 出力先の幅（ピクセル数）
	viewport.Height = window_height; // 出力先の高さ（ピクセル数）
	viewport.TopLeftX = 0; // 出力先の左上座標 X
	viewport.TopLeftY = 0; // 出力先の左上座標 Y
	viewport.MaxDepth = 1.0f; // 深度最大値
	viewport.MinDepth = 0.0f; // 深度最小値

	// シザー矩形
	D3D12_RECT scissorRect{};
	scissorRect.top = 0;                // 切り抜き上座標
	scissorRect.left = 0;               // 切り抜き左座標
	scissorRect.right = window_width;   // 切り抜き右座標
	scissorRect.bottom = window_height; // 切り抜きした座標

	float angle = 0;

	// ループ
	MSG msg{};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// アプリケーションが終わるときに message が WM_QUIT になる
		if (msg.message == WM_QUIT)
		{
			break;
		}

		angle += 0.01f;
		worldMat = XMMatrixRotationY(angle);
		*mapMat = worldMat * viewMat * projMat;

		// 現在のバックバッファーのインデックスを取得
		auto bbIdx = _swapChain->GetCurrentBackBufferIndex();

		{
			//D3D12_RESOURCE_BARRIER barrierDesc{};
			//barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; // 遷移
			//barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE; // 特に指定なし
			//barrierDesc.Transition.pResource = _backBuffers[bbIdx].Get();
			//barrierDesc.Transition.Subresource = 0;
			//barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT; // 直前は PRESENT 状態
			//barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; // 今からレンダーターゲット状態
			//_cmdList->ResourceBarrier(1, &barrierDesc);
		}

		_cmdList->ResourceBarrier(
			1, &CD3DX12_RESOURCE_BARRIER::Transition(
				_backBuffers[bbIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// レンダーターゲットビューをセット
		auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// レンダーターゲットのクリア
		float clearColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f }; // 背景色（今は黄色）
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		// ビューポートをセット
		_cmdList->RSSetViewports(1, &viewport);

		// シザー矩形をセット
		_cmdList->RSSetScissorRects(1, &scissorRect);

		// パイプラインをセット
		_cmdList->SetPipelineState(_pipelineState.Get());

		// ルートシグネチャをセット
		_cmdList->SetGraphicsRootSignature(rootSignature.Get());

		// プリミティブトポロジをセット
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 頂点バッファをセット
		_cmdList->IASetVertexBuffers(0, 1, &vbView);

		// インデックスバッファをセット
		_cmdList->IASetIndexBuffer(&ibView);

		// ルートシグネチャをセット
		_cmdList->SetGraphicsRootSignature(rootSignature.Get());

		// ディスクリプタヒープをセット
		_cmdList->SetDescriptorHeaps(1, basicDescHeap.GetAddressOf());

		// ルートパラメーターとディスクリプタヒープの関連付け
		_cmdList->SetGraphicsRootDescriptorTable(
			0, // ルートパラメーターインデックス
			basicDescHeap->GetGPUDescriptorHandleForHeapStart()); // ヒープアドレス

		// ドローコール
		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		{
			//barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			//barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
			//_cmdList->ResourceBarrier(1, &barrierDesc);
		}

		_cmdList->ResourceBarrier(
			1, &CD3DX12_RESOURCE_BARRIER::Transition(
				_backBuffers[bbIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		// コマンドリスト実行
		_cmdList->Close();

		ID3D12CommandList* cmdLists[]{ _cmdList.Get() };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);

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

		// コマンドリストなどのクリア
		_cmdAllocator->Reset(); // キューのクリア
		_cmdList->Reset(_cmdAllocator.Get(), nullptr); // 再びコマンドリストをためる準備

		// フリップ
		_swapChain->Present(1, 0);
	}

	// もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}

