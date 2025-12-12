#pragma once

#include "PrecompiledHeader.h"

#include <unordered_map>
#include <shared_mutex>
#include <atomic>

class CommandQueue
{
	friend class CommandQueueManager;
public:
	// 增加围栏值并返回新值
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

private:
	CommandQueue(uint64_t commandQueueId, ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE commandListType);

	const uint64_t m_kCommandQueueId;
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;

	std::mutex m_fenceMutex;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	uint64_t m_fenceValue;
	uint64_t m_completedFenceValueCache;
};

class CommandQueueManager
{
	friend class FenceTracker;

public:
	CommandQueueManager(ID3D12Device* pDevice);

	uint64_t CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);
	CommandQueue& GetCommandQueue(uint64_t commandQueueId);

private:
	static std::atomic_uint64_t sm_nextCommandQueueId;
	static std::shared_mutex sm_commandQueueMapMutex;
	static std::unordered_map<uint64_t, std::unique_ptr<CommandQueue>> sm_commandQueueMap;

	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	std::unordered_map<uint64_t, CommandQueue*> m_owningCommandQueueMap;
};

// 围栏追踪器
class FenceTracker
{
public:
	void SetPendingFenceValue(uint64_t commandQueueId, uint64_t fenceValue);
	bool ArePendingFencesCompleted();

private:
	std::unordered_map<uint64_t, uint64_t> m_pendingFences;
};
