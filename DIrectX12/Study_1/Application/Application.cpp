#include "Application.h"

#include <tchar.h>
#include "../Dx12Wrapper/Dx12Wrapper.h"
#include "../PMD/PMDActor.h"
#include "../PMD/PMDRenderer.h"

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

Application* Application::m_instance{ nullptr };

// 面倒だけど書かなければならない関数
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0); // OSに対して「もうこのアプリは終わる」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Application::CreateGameWindow(HWND & hwnd, WNDCLASSEX & windowClass)
{
	HINSTANCE hInstance = GetModuleHandle(nullptr);
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure; // コールバック関数の指定
	windowClass.lpszClassName = _T("DirectXTest"); // アプリケーションクラス名（適当でよい）
	windowClass.hInstance = hInstance; // ハンドルの取得

	RegisterClassEx(&windowClass);

	RECT wrc = { 0, 0, window_width, window_height }; // ウィンドウサイズを決める

	// 関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクトの作成
	m_hwnd = CreateWindow(
		windowClass.lpszClassName, // クラス名指定
		_T("DX12テスト"),          // タイトルバーの文字
		WS_OVERLAPPEDWINDOW,       // タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,             // 表示 x 座標は OS にお任せ
		CW_USEDEFAULT,             // 表示 y 座標は OS にお任せ
		wrc.right - wrc.left,      // ウィンドウ幅
		wrc.bottom - wrc.top,      // ウィンドウ高
		nullptr,                   // 親ウィンドウハンドル
		nullptr,                   // メニューハンドル
		windowClass.hInstance,     // 呼び出しアプリケーションハンドル
		nullptr);                  // 追加パラメーター
}

Application::Application()
{
}

Application & Application::Instance()
{
	static Application instance;
	return instance;
}

bool Application::Init()
{
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
	CreateGameWindow(m_hwnd, m_windowClass);

	// DirectX12ラッパー生成＆初期化
	m_dx12 = std::make_shared<Dx12Wrapper>(m_hwnd);
	m_pmdRenderer = std::make_shared<PMDRenderer>(m_dx12);
	//m_pmdActor.reset(new PMDActor("Model/初音ミク.pmd", m_dx12));

	// make_sharedだとなぜかできない。
	//auto miku = std::make_shared<PMDActor>("Model/初音ミク.pmd", m_dx12);
	
	auto miku = std::shared_ptr<PMDActor>(new PMDActor("Model/初音ミク.pmd", m_dx12));
	miku->LoadVMDFile("motion/pose.vmd");
	miku->SetPos(-10.0f, 0.0f, 0.0f);
	m_pmdRenderer->AddActor(miku);

	auto ruka = std::shared_ptr<PMDActor>(new PMDActor("Model/巡音ルカ.pmd", m_dx12));
	ruka->LoadVMDFile("motion/pose.vmd");
	m_pmdRenderer->AddActor(ruka);

	auto haku = std::shared_ptr<PMDActor>(new PMDActor("Model/弱音ハク.pmd", m_dx12));
	haku->LoadVMDFile("motion/pose.vmd");
	haku->SetPos(-5.0f, 0.0f, 5.0f);
	m_pmdRenderer->AddActor(haku);

	auto rin = std::shared_ptr<PMDActor>(new PMDActor("Model/鏡音リン.pmd", m_dx12));
	rin->LoadVMDFile("motion/pose.vmd");
	rin->SetPos(10.0f, 0.0f, 10.0f);
	m_pmdRenderer->AddActor(rin);

	auto ren = std::shared_ptr<PMDActor>(new PMDActor("Model/鏡音レン.pmd", m_dx12));
	ren->LoadVMDFile("motion/pose.vmd");
	ren->SetPos(5.0f, 0.0f, 5.0f);
	m_pmdRenderer->AddActor(ren);

	auto meiko = std::shared_ptr<PMDActor>(new PMDActor("Model/咲音メイコ.pmd", m_dx12));
	meiko->LoadVMDFile("motion/pose.vmd");
	meiko->SetPos(-10.0f, 0.0f, 10.0f);
	m_pmdRenderer->AddActor(meiko);

	auto kaito = std::shared_ptr<PMDActor>(new PMDActor("Model/カイト.pmd", m_dx12));
	kaito->LoadVMDFile("motion/pose.vmd");
	kaito->SetPos(10.0f, 0.0f, 0.0f);
	m_pmdRenderer->AddActor(kaito);

	return true;
}

void Application::Run()
{
	float angle = 0.0f;
	ShowWindow(m_hwnd, SW_SHOW);

	MSG msg{};
	unsigned int frame = 0;

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// アプリケーションが終わるときに message は WM_QUIT になる
		if (msg.message == WM_QUIT)
		{
			break;
		}

		m_dx12->PreDrawToPera1();
		m_pmdRenderer->Update();
		m_pmdRenderer->BeforeDraw();
		m_dx12->SetCameraInfo(); // 元はここだった
		m_pmdRenderer->Draw();
		m_dx12->DrawHorizontalBokeh();
		m_dx12->Clear();
		//m_dx12->DrawToPera1();
		m_dx12->Draw();

		m_dx12->Flip();
	}
}

void Application::Terminate()
{
	// もうクラスは使わないので登録解除する
	UnregisterClass(m_windowClass.lpszClassName, m_windowClass.hInstance);
}

SIZE Application::GetWindowSize() const
{
	SIZE ret;
	ret.cx = window_width;
	ret.cy = window_height;
	return ret;
}

Application::~Application()
{

}
