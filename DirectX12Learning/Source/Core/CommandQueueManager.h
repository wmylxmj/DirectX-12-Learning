#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueue.h"

#include <unordered_map>
#include <shared_mutex>
#include <atomic>

class CommandQueue
{
public:
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
