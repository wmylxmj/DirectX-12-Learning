#pragma once

#include "Resource.h"
#include "PrecompiledHeader.h"

class UploadBuffer : Resource
{
public:
	UploadBuffer(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, size_t bufferSize);
};
