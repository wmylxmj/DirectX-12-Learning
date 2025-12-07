#pragma once

#include "PrecompiledHeader.h"

class Device
{
public:
	Device(IUnknown* pAdapter);

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
};