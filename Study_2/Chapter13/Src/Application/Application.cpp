#include "Application.h"

#include "../Dx12Wrapper/Dx12Wrapper.h"
#include "../PMD/PMDRenderer.h"
#include "../PMD/PMDActor.h"

// �ʓ|�����Ǐ����Ȃ���΂����Ȃ��֐�
HRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// �E�B���h�E���j�����ꂽ��Ă΂��
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0); // OS�ɑ΂��āu�������̃A�v���͏I���v�Ɠ`����
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam); // ����̏������s��
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
	// �E�B���h�E�쐬
	CreateGameWindow(m_hwnd, m_windowClass, windowWidth, windowHeight);

	// DirectX12�̃��b�p�[�N���X�̍쐬
	m_dx12 = std::make_shared<Dx12Wrapper>();

	// PMD��\������N���X�̍쐬
	m_pmdRenderer = std::make_shared<PMDRenderer>(m_dx12);

	// PMD���f��
	m_pmdActor = std::make_shared<PMDActor>("Model/�����~�N.pmd", m_dx12);

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
			// ���C�����[�v���甲����
			break;
		}

		// DirectX12�̏���
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
	// �����N���X�͎g��Ȃ��̂œo�^��������
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
	// �E�B���h�E�T�C�Y��ۑ�
	m_windowWidth = windowWidth;
	m_windowHeight = windowHeight;

	// �E�B���h�E�N���X�̐������o�^
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure; // �R�[���o�b�N�֐��̎w��
	windowClass.lpszClassName = "DX12Sample";       // �A�v���P�[�V�����N���X���i�K���ł悢�j
	windowClass.hInstance = GetModuleHandle(nullptr);   // �n���h���̎擾

	RegisterClassEx(&windowClass); // �A�v���P�[�V�����N���X�i�E�B���h�E�N���X�̎w���OS�ɓ`����j

	RECT wrc = { 0, 0, windowWidth, windowHeight }; // �E�B���h�E�T�C�Y�����߂�

	// �֐����g���ăE�B���h�E�̃T�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E�I�u�W�F�N�g�̐���
	hwnd = CreateWindow(
		windowClass.lpszClassName,      // �N���X���w��
		"DX12�e�X�g",	      // �^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,  // �^�C�g���o�[�Ƌ��E������E�B���h�E
		CW_USEDEFAULT,		  // �\�� X ���W��OS�ɂ��C��
		CW_USEDEFAULT,		  // �\�� Y ���W��OS�ɂ��C��
		wrc.right - wrc.left, // �E�B���h�E��
		wrc.bottom - wrc.top, // �E�B���h�E��
		nullptr,			  // �e�E�B���h�E�n���h��
		nullptr,			  // ���j���[�n���h��
		windowClass.hInstance,		  // �Ăяo���A�v���P�[�V�����n���h��
		nullptr);			  // �ǉ��p�����[�^�[

	// �E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);
}
