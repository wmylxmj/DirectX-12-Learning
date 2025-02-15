#pragma once

#include <comdef.h> // 用于处理 HRESULT 错误

// ----------------DirectX 12--------------
#include <dxgi1_6.h>
#include <d3d12.h>
#include <D3Dcompiler.h>

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
// ----------------------------------------

#include <string>

#ifndef CHECK_HRESULT
#define CHECK_HRESULT(x) \
{ \
	HRESULT hr = (x); \
	if (FAILED(hr)) \
	{ \
	_com_error err(hr); \
	std::wstring wError = err.ErrorMessage(); \
	std::wstring wErrMsg = std::wstring(L ## #x) + \
	L" failed in " + std::wstring(__FILEW__) + \
	L"; line: " + std::to_wstring(__LINE__) + \
	L"; error: " + wError; \
	MessageBox(0, wErrMsg.c_str(), L"HRESULT 错误", 0); \
	} \
}                        
#endif


