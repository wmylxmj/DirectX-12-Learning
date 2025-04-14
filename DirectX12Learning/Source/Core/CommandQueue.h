#pragma once

#include <mutex>
#include <atomic>

#include "PrecompiledHeader.h"
#include "CommandAllocatorPool.h"
#include "CommandListPool.h"

class CommandQueue
{
private:
	static std::atomic_uint64_t sm_nextNonReusableId;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;

	std::mutex m_fenceMutex;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	uint64_t m_fenceValue;
	uint64_t m_completedFenceValueCache;

	CommandAllocatorPool m_commandAllocatorPool;
	CommandListPool m_commandListPool;

public:
	CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, D3D12_COMMAND_LIST_TYPE commandListType);

	uint64_t IncreaseFenceValue();
	bool IsFenceValueCompleted(uint64_t fenceValue);
	void StallForAnotherQueueFence(const CommandQueue& queue, uint64_t fenceValue);
	void StallForAnotherQueueCompletion(const CommandQueue& queue);
	void WaitForFence(uint64_t fenceValue);
	void WaitForIdle();

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
	uint64_t GetFenceValue() const;

	uint64_t ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList);

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> RequestCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device> pDevice);
	void DiscardCommandAllocator(uint64_t fenceValueForReset, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator);

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> RequestCommandList(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator);
	void DiscardCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList);
};
