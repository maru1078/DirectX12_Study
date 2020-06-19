#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <Windows.h>
#include <memory>

class Dx12Wrapper;
class PMDRenderer;
class PMDActor;

class Application
{
public:	
	
	~Application();

	static Application& Instance();

	// 初期化
	bool Init(float windowWidth, float windowHeight);

	// ループ処理
	void Run();

	// 後処理
	void Teminate();
	
	const HWND& GetHWND() const;

	UINT WindowWidth() const;
	UINT WindowHeight() const;

private:

	void CreateGameWindow(HWND& hwnd, WNDCLASSEX& windowClass, UINT windowWidth, UINT windowHeight);

private:

	// コンストラクタをprivateに
	Application() = default;

	// コピー禁止
	Application(const Application&) = delete;
	void operator = (const Application&) = delete;

private:

	WNDCLASSEX m_windowClass;
	HWND m_hwnd;
	UINT m_windowWidth;
	UINT m_windowHeight;

	std::shared_ptr<Dx12Wrapper> m_dx12;
	std::shared_ptr<PMDRenderer> m_pmdRenderer;
	std::shared_ptr<PMDActor> m_pmdActor;
};

#endif // !APPLICATION_H_

