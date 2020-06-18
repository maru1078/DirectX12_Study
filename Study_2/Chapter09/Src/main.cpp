#include <Windows.h>

#include "Application/Application.h"
#include "Helper/Helper.h"

#ifdef _DEBUG

int main()
{

#else

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

#endif

	DebugOutputFormatString("Show window test.");

	auto& app = Application::Instance();
	app.Init(960, 540);

	// ÉãÅ[Év
	app.Run();

	app.Teminate();

	return 0;
}

