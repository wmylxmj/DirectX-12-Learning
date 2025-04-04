#pragma once

// Use the C++ standard templated min/max
#define NOMINMAX

#include <Windows.h>

#include <wrl.h> // 用于 Microsoft 的智能指针
#include <comdef.h> // 用于处理 HRESULT 错误

// ----------------DirectX 12--------------
#include <dxgi1_6.h>
#include <d3d12.h>
#include <D3Dcompiler.h>

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#include <DirectXMath.h>
#include "d3dx12.h"

#include <DirectXColors.h>
// -----------------------------------------

#include <string>

#ifndef CHECK_HRESULT
#define CHECK_HRESULT(x) \
{ \
	HRESULT CHECK_HRESULT_hr = (x); \
	if (FAILED(CHECK_HRESULT_hr)) \
	{ \
		_com_error err(CHECK_HRESULT_hr); \
		std::wstring wError = err.ErrorMessage(); \
		std::wstring wErrMsg = std::wstring(L ## #x) + \
		L" failed in " + std::wstring(__FILEW__) + \
		L"; line: " + std::to_wstring(__LINE__) + \
		L"; error: " + wError; \
		MessageBox(0, wErrMsg.c_str(), L"♥杂鱼～♥ HRESULT 出错啦", 0); \
	} \
}
#endif

template <typename T>
inline T& RvalueToLvalue(T&& value) {
	return value; // 直接返回引用
}
