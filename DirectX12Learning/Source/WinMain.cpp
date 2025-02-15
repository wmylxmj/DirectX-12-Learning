#include "Windows.h"
#include "WindowsX.h"

#include <wrl.h> // ���� Microsoft ������ָ��
#include <comdef.h> // ���ڴ��� HRESULT ����

// ----------------DirectX 12--------------
#include <dxgi1_6.h>
#include <d3d12.h>
#include <D3Dcompiler.h>

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#include <DirectXMath.h>
// -----------------------------------------

#include <string>

// �Լ���ͷ�ļ�
#include "D3D12/Helper.h"

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
) {
	OutputDebugStringA("Hello World!\n");
	return 0;
}
