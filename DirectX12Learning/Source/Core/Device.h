#pragma once

#include "PrecompiledHeader.h"
#include "CommandAllocatorPool.h"

#include <unordered_map>
#include <mutex>

class Device
{
public:
	Device(IUnknown* pAdapter);

	ID3D12Device* GetDevice() const;

	ID3D12CommandAllocator* RequestCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType);
	void DiscardCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, FenceTracker fenceTracker, ID3D12CommandAllocator* pCommandAllocator);

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;

	std::unique_ptr<CommandQueueManager> m_pCommandQueueManager;

	std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::unique_ptr<CommandAllocatorPool>> m_commandAllocatorPoolMap;

	std::mutex m_rootSignatureCacheMutex;
};