#include "Application.h"

#include "../Dx12Wrapper/Dx12Wrapper.h"
#include "../PMD/PMDRenderer.h"
#include "../PMD/PMDActor.h"

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

Application::~Application()
{
}

Application & Application::Instance()
{
	static Application instance;

	return instance;
}

bool Application::Init(float windowWidth, float windowHeight)
{
	// ウィンドウ作成
	CreateGameWindow(m_hwnd, m_windowClass, windowWidth, windowHeight);

	// DirectX12のラッパークラスの作成
	m_dx12 = std::make_shared<Dx12Wrapper>();

	// PMDを表示するクラスの作成
	m_pmdRenderer = std::make_shared<PMDRenderer>(m_dx12);

	// PMDモデル
	m_pmdActor = std::make_shared<PMDActor>("Model/初音ミク.pmd", m_dx12);

	return true;
}

void Application::Run()
{
	MSG msg{};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
		{
			// メインループから抜ける
			break;
		}

		// DirectX12の処理
		m_dx12->BeginDraw();
		m_pmdRenderer->PreDrawPMD();
		m_dx12->SetSceneMat();
		m_pmdActor->Update();
		m_pmdActor->Draw();
		m_dx12->DrawPeraPolygon();
		//m_dx12->DrawPera2Polygon();
		m_dx12->EndDraw();
	}
}

void Application::Teminate()
{
	// もうクラスは使わないので登録解除する
	UnregisterClass(m_windowClass.lpszClassName, m_windowClass.hInstance);
}

const HWND & Application::GetHWND() const
{
	return m_hwnd;
}

UINT Application::WindowWidth() const
{
	return m_windowWidth;
}

UINT Application::WindowHeight() const
{
	return m_windowHeight;
}

void Application::CreateGameWindow(HWND & hwnd, WNDCLASSEX & windowClass, UINT windowWidth, UINT windowHeight)
{
	// ウィンドウサイズを保存
	m_windowWidth = windowWidth;
	m_windowHeight = windowHeight;

	// ウィンドウクラスの生成＆登録
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure; // コールバック関数の指定
	windowClass.lpszClassName = "DX12Sample";       // アプリケーションクラス名（適当でよい）
	windowClass.hInstance = GetModuleHandle(nullptr);   // ハンドルの取得

	RegisterClassEx(&windowClass); // アプリケーションクラス（ウィンドウクラスの指定をOSに伝える）

	RECT wrc = { 0, 0, windowWidth, windowHeight }; // ウィンドウサイズを決める

	// 関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクトの生成
	hwnd = CreateWindow(
		windowClass.lpszClassName,      // クラス名指定
		"DX12テスト",	      // タイトルバーの文字
		WS_OVERLAPPEDWINDOW,  // タイトルバーと境界があるウィンドウ
		CW_USEDEFAULT,		  // 表示 X 座標はOSにお任せ
		CW_USEDEFAULT,		  // 表示 Y 座標はOSにお任せ
		wrc.right - wrc.left, // ウィンドウ幅
		wrc.bottom - wrc.top, // ウィンドウ高
		nullptr,			  // 親ウィンドウハンドル
		nullptr,			  // メニューハンドル
		windowClass.hInstance,		  // 呼び出しアプリケーションハンドル
		nullptr);			  // 追加パラメーター

	// ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);
}
