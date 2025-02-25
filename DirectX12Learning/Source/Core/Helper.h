#pragma once

#include "PrecompiledHeader.h"

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

