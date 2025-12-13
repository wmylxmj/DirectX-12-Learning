#include "Device.h"

Device::Device(IUnknown* pAdapter)
{
	CHECK_HRESULT(D3D12CreateDevice(
		pAdapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_pDevice)
	));

	m_pCommandQueueManager = std::make_unique<CommandQueueManager>(m_pDevice.Get());
	m_pCommandQueueManager->CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

	m_commandAllocatorPoolMap.emplace(D3D12_COMMAND_LIST_TYPE_DIRECT, std::make_unique<CommandAllocatorPool>(m_pDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT));
	m_commandAllocatorPoolMap.emplace(D3D12_COMMAND_LIST_TYPE_COMPUTE, std::make_unique<CommandAllocatorPool>(m_pDevice.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE));
	m_commandAllocatorPoolMap.emplace(D3D12_COMMAND_LIST_TYPE_COPY, std::make_unique<CommandAllocatorPool>(m_pDevice.Get(), D3D12_COMMAND_LIST_TYPE_COPY));
}

ID3D12Device* Device::GetDevice() const
{
	return m_pDevice.Get();
}

uint64_t Device::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
	return m_pCommandQueueManager->CreateCommandQueue(commandListType);
}

ID3D12CommandAllocator* Device::RequestCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType)
{
	return m_commandAllocatorPoolMap[commandListType]->RequestCommandAllocator();
}

void Device::DiscardCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, FenceTracker fenceTracker, ID3D12CommandAllocator* pCommandAllocator)
{
	m_commandAllocatorPoolMap[commandListType]->DiscardCommandAllocator(fenceTracker, pCommandAllocator);
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> Device::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	return Microsoft::WRL::ComPtr<ID3D12RootSignature>();
}