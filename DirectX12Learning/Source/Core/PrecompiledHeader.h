#pragma once

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

