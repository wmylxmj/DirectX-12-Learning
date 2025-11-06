#pragma once

#include "PrecompiledHeader.h"

#include <vector>
#include <queue>
#include <mutex>

class CommandAllocatorPool
{
public:
	CommandAllocatorPool(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type);

	ID3D12CommandAllocator* RequestCommandAllocator(uint64_t completedFenceValue);
	void DiscardCommandAllocator(uint64_t fenceValueForReset, ID3D12CommandAllocator* pCommandAllocator);

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;

	std::mutex m_poolMutex;

	std::mutex m_allocatorMutex;
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_commandAllocatorPool;

	std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_retiredCommandAllocators;
};