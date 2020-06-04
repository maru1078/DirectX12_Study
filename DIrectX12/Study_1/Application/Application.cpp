#include "Application.h"

#include <tchar.h>
#include "../Dx12Wrapper/Dx12Wrapper.h"
#include "../PMD/PMDActor.h"
#include "../PMD/PMDRenderer.h"

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

Application* Application::m_instance{ nullptr };

// �ʓ|�����Ǐ����Ȃ���΂Ȃ�Ȃ��֐�
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// �E�B���h�E���j�����ꂽ��Ă΂��
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0); // OS�ɑ΂��āu�������̃A�v���͏I���v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Application::CreateGameWindow(HWND & hwnd, WNDCLASSEX & windowClass)
{
	HINSTANCE hInstance = GetModuleHandle(nullptr);
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure; // �R�[���o�b�N�֐��̎w��
	windowClass.lpszClassName = _T("DirectXTest"); // �A�v���P�[�V�����N���X���i�K���ł悢�j
	windowClass.hInstance = hInstance; // �n���h���̎擾

	RegisterClassEx(&windowClass);

	RECT wrc = { 0, 0, window_width, window_height }; // �E�B���h�E�T�C�Y�����߂�

	// �֐����g���ăE�B���h�E�̃T�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E�I�u�W�F�N�g�̍쐬
	m_hwnd = CreateWindow(
		windowClass.lpszClassName, // �N���X���w��
		_T("DX12�e�X�g"),          // �^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,       // �^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,             // �\�� x ���W�� OS �ɂ��C��
		CW_USEDEFAULT,             // �\�� y ���W�� OS �ɂ��C��
		wrc.right - wrc.left,      // �E�B���h�E��
		wrc.bottom - wrc.top,      // �E�B���h�E��
		nullptr,                   // �e�E�B���h�E�n���h��
		nullptr,                   // ���j���[�n���h��
		windowClass.hInstance,     // �Ăяo���A�v���P�[�V�����n���h��
		nullptr);                  // �ǉ��p�����[�^�[
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

	// DirectX12���b�p�[������������
	m_dx12 = std::make_shared<Dx12Wrapper>(m_hwnd);
	m_pmdRenderer = std::make_shared<PMDRenderer>(m_dx12);
	//m_pmdActor.reset(new PMDActor("Model/�����~�N.pmd", m_dx12));

	// make_shared���ƂȂ����ł��Ȃ��B
	//auto miku = std::make_shared<PMDActor>("Model/�����~�N.pmd", m_dx12);
	
	auto miku = std::shared_ptr<PMDActor>(new PMDActor("Model/�����~�N.pmd", m_dx12));
	miku->LoadVMDFile("motion/pose.vmd");
	miku->SetPos(-10.0f, 0.0f, 0.0f);
	m_pmdRenderer->AddActor(miku);

	auto ruka = std::shared_ptr<PMDActor>(new PMDActor("Model/�������J.pmd", m_dx12));
	ruka->LoadVMDFile("motion/pose.vmd");
	m_pmdRenderer->AddActor(ruka);

	auto haku = std::shared_ptr<PMDActor>(new PMDActor("Model/�㉹�n�N.pmd", m_dx12));
	haku->LoadVMDFile("motion/pose.vmd");
	haku->SetPos(-5.0f, 0.0f, 5.0f);
	m_pmdRenderer->AddActor(haku);

	auto rin = std::shared_ptr<PMDActor>(new PMDActor("Model/��������.pmd", m_dx12));
	rin->LoadVMDFile("motion/pose.vmd");
	rin->SetPos(10.0f, 0.0f, 10.0f);
	m_pmdRenderer->AddActor(rin);

	auto ren = std::shared_ptr<PMDActor>(new PMDActor("Model/��������.pmd", m_dx12));
	ren->LoadVMDFile("motion/pose.vmd");
	ren->SetPos(5.0f, 0.0f, 5.0f);
	m_pmdRenderer->AddActor(ren);

	auto meiko = std::shared_ptr<PMDActor>(new PMDActor("Model/�特���C�R.pmd", m_dx12));
	meiko->LoadVMDFile("motion/pose.vmd");
	meiko->SetPos(-10.0f, 0.0f, 10.0f);
	m_pmdRenderer->AddActor(meiko);

	auto kaito = std::shared_ptr<PMDActor>(new PMDActor("Model/�J�C�g.pmd", m_dx12));
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

		// �A�v���P�[�V�������I���Ƃ��� message �� WM_QUIT �ɂȂ�
		if (msg.message == WM_QUIT)
		{
			break;
		}

		m_dx12->PreDrawToPera1();
		m_pmdRenderer->Update();
		m_pmdRenderer->BeforeDraw();
		m_dx12->SetCameraInfo(); // ���͂���������
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
	// �����N���X�͎g��Ȃ��̂œo�^��������
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
