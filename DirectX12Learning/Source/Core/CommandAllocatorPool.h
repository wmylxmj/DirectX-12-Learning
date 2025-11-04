#pragma once

#include "PrecompiledHeader.h"

#include <queue>
#include <mutex>

class CommandAllocatorPool
{
public:
	CommandAllocatorPool(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type);

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> RequestCommandAllocator(uint64_t completedFenceValue);
	void DiscardCommandAllocator(uint64_t fenceValueForReset, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator);

private:
	std::queue<std::pair<uint64_t, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>> m_readyAllocators;
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;

	std::mutex m_allocatorMutex;
};