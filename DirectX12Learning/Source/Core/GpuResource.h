#pragma once

#include <wrl.h>
#include "PrecompiledHeader.h"

class GpuResource
{
protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;
	D3D12_RESOURCE_STATES m_resourceState;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuVirtualAddress;
};
