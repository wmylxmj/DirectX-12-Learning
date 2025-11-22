#pragma once

#include <mutex>
#include <atomic>

#include "PrecompiledHeader.h"
#include "CommandAllocatorPool.h"
#include "CommandListPool.h"
#include "Fence.h"

class CommandQueue
{
public:
	CommandQueue(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE commandListType);

	uint64_t IncrementFenceValue();
	bool IsFenceValueCompleted(uint64_t fenceValue);

	void StallForAnotherQueueFence(const CommandQueue& queue, uint64_t fenceValue);
	void StallForAnotherQueueCompletion(const CommandQueue& queue);

	void WaitForFenceValue(uint64_t fenceValue);
	void WaitForIdle();

	ID3D12CommandQueue* GetCommandQueue() const;
	uint64_t GetCommandQueueId() const;
	uint64_t GetFenceValue() const;

	uint64_t ExecuteCommandList(ID3D12GraphicsCommandList* pCommandList);

	ID3D12CommandAllocator* RequestCommandAllocator();
	void DiscardCommandAllocator(uint64_t fenceValueForReset, ID3D12CommandAllocator* pCommandAllocator);

	ID3D12GraphicsCommandList* RequestCommandList(ID3D12CommandAllocator* pCommandAllocator);
	void DiscardCommandList(ID3D12GraphicsCommandList* pCommandList);

private:
	// √¸¡Ó∂”¡–id
	const uint64_t m_kCommandQueueId;

	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;

	std::mutex m_fenceMutex;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	uint64_t m_fenceValue;
	uint64_t m_completedFenceValueCache;

	CommandAllocatorPool m_commandAllocatorPool;
	CommandListPool m_commandListPool;
};
