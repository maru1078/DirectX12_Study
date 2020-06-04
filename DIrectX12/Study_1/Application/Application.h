#pragma once

#include <Windows.h>
#include <memory>

class Dx12Wrapper;
class PMDRenderer;
class PMDActor;

class Application
{
private:

	// �����ɕK�v�ȕϐ��i�o�b�t�@�[��q�[�v�Ȃǁj������

	// �E�B���h�E����
	WNDCLASSEX m_windowClass;
	HWND m_hwnd;
	std::shared_ptr<Dx12Wrapper> m_dx12;
	std::shared_ptr<PMDRenderer> m_pmdRenderer;
	std::shared_ptr<PMDActor> m_pmdActor;

private:
	// �֐�

	// �Q�[���p�E�B���h�E�̐���
	void CreateGameWindow(HWND& hwnd, WNDCLASSEX& windowClass);

private:
	// �V���O���g���̂��߂ɃR���X�g���N�^��private��
	// ����ɃR�s�[�Ƒ�����֎~�ɂ���
	Application();
	Application(const Application&) = delete;
	void operator =(const Application&) = delete;

	static Application* m_instance;

public:

	// Application�̃V���O���g���C���X�^���X�𓾂�
	static Application& Instance();

	// ������
	bool Init();

	// ���[�v�N��
	void Run();

	// �㏈��
	void Terminate();
	SIZE GetWindowSize()const;
	~Application();

};
