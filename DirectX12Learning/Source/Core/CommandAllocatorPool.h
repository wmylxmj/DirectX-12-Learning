#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>

#include "wrl.h"

class CommandAllocatorPool
{
private:
	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_allocatorPool;
	std::queue<std::pair<uint64_t, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>> m_readyAllocators;
	std::mutex m_allocatorMutex;
	const D3D12_COMMAND_LIST_TYPE m_commandListType;

public:
	CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type);

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> RequestAllocator(Microsoft::WRL::ComPtr<ID3D12Device> device, uint64_t completedFenceValue);

	void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator);

};