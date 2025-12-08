#include "Device.h"

Device::Device(IUnknown* pAdapter)
{
	CHECK_HRESULT(D3D12CreateDevice(
		pAdapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_pDevice)
	));

	m_commandAllocatorPoolMap.emplace(D3D12_COMMAND_LIST_TYPE_DIRECT, std::make_unique<CommandAllocatorPool>(m_pDevice.Get()));
	m_commandAllocatorPoolMap.emplace(D3D12_COMMAND_LIST_TYPE_COMPUTE, std::make_unique<CommandAllocatorPool>(m_pDevice.Get()));
	m_commandAllocatorPoolMap.emplace(D3D12_COMMAND_LIST_TYPE_COPY, std::make_unique<CommandAllocatorPool>(m_pDevice.Get()));
}