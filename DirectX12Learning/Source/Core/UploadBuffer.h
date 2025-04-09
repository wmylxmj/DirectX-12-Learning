#pragma once

#include "Resource.h"
#include "PrecompiledHeader.h"

class UploadBuffer : Resource
{
protected:
	size_t m_bufferSize;
public:
	UploadBuffer(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, size_t bufferSize);

	void* Map();
};
