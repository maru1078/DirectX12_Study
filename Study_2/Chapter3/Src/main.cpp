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
	for (int i = 0; i < swcDesc.BufferCount; ++i)
	{
		// スワップチェーン作成時に確保したバッファを取得
		result = _swapChain->GetBuffer(i, IID_PPV_ARGS(_backBuffers[i].ReleaseAndGetAddressOf()));

		// 取得したバッファのなかにRTVを作成する（っていう流れかな？）
		_dev->CreateRenderTargetView(_backBuffers[i].Get(), nullptr, handle);

		// そのままだと同じ個所をさしてしまうのでアドレスをずらす
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

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

		// 現在のバックバッファーのインデックスを取得
		auto bbIdx = _swapChain->GetCurrentBackBufferIndex();

		D3D12_RESOURCE_BARRIER barrierDesc{};
		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; // 遷移
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE; // 特に指定なし
		barrierDesc.Transition.pResource = _backBuffers[bbIdx].Get();
		barrierDesc.Transition.Subresource = 0;
		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT; // 直前は PRESENT 状態
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; // 今からレンダーターゲット状態
		_cmdList->ResourceBarrier(1, &barrierDesc);

		// レンダーターゲットビューをセット
		auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		// レンダーターゲットのクリア
		float clearColor[4]{ 1.0f, 1.0f, 0.0f, 1.0f }; // 背景色（今は黄色）
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &barrierDesc);

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

