#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueueManager.h"

#include <vector>
#include <queue>
#include <mutex>

class CommandAllocatorPool
{
public:

	CommandAllocatorPool(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE commandListType);

	ID3D12CommandAllocator* RequestCommandAllocator();

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;

	std::mutex m_poolMutex;
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_commandAllocatorPool;
	std::queue<ID3D12CommandAllocator*> m_availableCommandAllocators;
	std::queue<std::pair<FenceTracker, ID3D12CommandAllocator*>> m_retiredCommandAllocators;
};