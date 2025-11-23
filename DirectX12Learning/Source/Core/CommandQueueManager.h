#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueue.h"

#include <unordered_map>
#include <atomic>

class CommandQueueManager
{
public:
	CommandQueueManager(ID3D12Device* pDevice);

	uint64_t CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);
	CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);

private:
	static std::atomic_uint64_t sm_nextCommandQueueId;
	static std::mutex sm_commandQueueMapMutex;
	static std::unordered_map<uint64_t, std::unique_ptr<CommandQueue>> sm_commandQueueMap;

	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	std::unordered_map<D3D12_COMMAND_LIST_TYPE, uint64_t> m_commandQueueIdMap;
};

// Î§À¸×·×ÙÆ÷
class FenceTracker
{
};