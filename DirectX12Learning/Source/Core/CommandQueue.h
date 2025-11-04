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
	CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, D3D12_COMMAND_LIST_TYPE commandListType);

	uint64_t IncrementFenceValue();
	bool IsFenceValueCompleted(uint64_t fenceValue);
	void StallForAnotherQueueFence(const CommandQueue& queue, uint64_t fenceValue);
	void StallForAnotherQueueCompletion(const CommandQueue& queue);
	void WaitForFence(uint64_t fenceValue);
	void WaitForIdle();

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const;
	uint64_t GetFenceValue() const;
	uint64_t GetNonReusableId() const;

	uint64_t ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList);

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> RequestCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device> pDevice);
	void DiscardCommandAllocator(uint64_t fenceValueForReset, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator);

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> RequestCommandList(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator);
	void DiscardCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList);

private:
	static std::atomic_uint64_t sm_nextNonReusableId;
	const uint64_t m_kNonReusableId;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;

	std::mutex m_fenceMutex;
	std::unique_ptr<Fence> m_pFence;

	CommandAllocatorPool m_commandAllocatorPool;
	CommandListPool m_commandListPool;
};
