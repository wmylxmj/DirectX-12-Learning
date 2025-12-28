#pragma once

#include "PrecompiledHeader.h"

#include <mutex>

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
	uint64_t GetCurrentFenceValue() const;

	// 根据命令分配器创建命令列表
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ID3D12CommandAllocator* pCommandAllocator);

	uint64_t ExecuteCommandList(ID3D12GraphicsCommandList* pCommandList);

	uint64_t GetCommandQueueId() const;
	D3D12_COMMAND_LIST_TYPE GetCommandListType() const;

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;

	std::mutex m_fenceMutex;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	uint64_t m_fenceValue;
	uint64_t m_completedFenceValueCache;
};
};