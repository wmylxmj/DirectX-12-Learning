#pragma once

#include <mutex>

#include "PrecompiledHeader.h"
#include "CommandAllocatorPool.h"
#include "wrl.h"

class CommandQueue
{
private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;

	std::mutex m_fenceMutex;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	uint64_t m_fenceValue;
	uint64_t m_completedFenceValueCache;

	CommandAllocatorPool m_commandAllocatorPool;

public:
	CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, D3D12_COMMAND_LIST_TYPE commandListType);

	uint64_t IncreaseFenceValue();
	bool IsFenceValueCompleted(uint64_t fenceValue);
	void StallForAnotherQueueFence(const CommandQueue& queue, uint64_t fenceValue);
	void StallForAnotherQueueCompletion(const CommandQueue& queue);
};