#pragma once

#include <Windows.h>
#include <memory>

class Dx12Wrapper;
class PMDRenderer;
class PMDActor;

class Application
{
private:

	// ここに必要な変数（バッファーやヒープなど）を書く

	// ウィンドウ周り
	WNDCLASSEX m_windowClass;
	HWND m_hwnd;
	std::shared_ptr<Dx12Wrapper> m_dx12;
	std::shared_ptr<PMDRenderer> m_pmdRenderer;
	std::shared_ptr<PMDActor> m_pmdActor;

private:
	// 関数

	// ゲーム用ウィンドウの生成
	void CreateGameWindow(HWND& hwnd, WNDCLASSEX& windowClass);

private:
	// シングルトンのためにコンストラクタをprivateに
	// さらにコピーと代入を禁止にする
	Application();
	Application(const Application&) = delete;
	void operator =(const Application&) = delete;

	static Application* m_instance;

public:

	// Applicationのシングルトンインスタンスを得る
	static Application& Instance();

	// 初期化
	bool Init();

	// ループ起動
	void Run();

	// 後処理
	void Terminate();
	SIZE GetWindowSize()const;
	~Application();

};
