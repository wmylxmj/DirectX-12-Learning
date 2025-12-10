#pragma once

#include "PrecompiledHeader.h"

#include <unordered_map>
#include <shared_mutex>
#include <atomic>

class CommandQueue
{
public:
	uint64_t IncrementFenceValue();

private:
	CommandQueue(uint64_t commandQueueId, ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE commandListType);

	const uint64_t m_commandQueueId;

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

// Î§À¸×·×ÙÆ÷
class FenceTracker
{
public:
	void SetPendingFenceValue(uint64_t commandQueueId, uint64_t fenceValue);
	bool ArePendingFencesCompleted();

private:
	std::unordered_map<uint64_t, uint64_t> m_pendingFences;
};
