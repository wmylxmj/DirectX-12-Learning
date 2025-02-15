#include "Windows.h"
#include "WindowsX.h"


int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
) {
	OutputDebugStringA("Hello World!\n");
	return 0;
}
