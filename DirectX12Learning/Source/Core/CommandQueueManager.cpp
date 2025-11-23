#include "CommandQueueManager.h"

// static 变量定义
std::atomic_uint64_t CommandQueueManager::sm_nextCommandQueueId = 1;
std::mutex CommandQueueManager::sm_commandQueueMapMutex;
std::unordered_map<uint64_t, std::unique_ptr<CommandQueue>> CommandQueueManager::sm_commandQueueMap;

CommandQueueManager::CommandQueueManager(ID3D12Device* pDevice)
	: m_pDevice(pDevice)
{
}

uint64_t CommandQueueManager::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
	std::lock_guard<std::mutex> lockGuard(sm_commandQueueMapMutex);

	uint64_t commandQueueId = sm_nextCommandQueueId++;
	sm_commandQueueMap.emplace(commandQueueId, std::make_unique<CommandQueue>(m_pDevice.Get(), commandListType));

	return commandQueueId;
}

CommandQueue& CommandQueueManager::GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
	std::lock_guard<std::mutex> lockGuard(sm_commandQueueMapMutex);
	return *sm_commandQueueMap.at(m_commandQueueIdMap.at(commandListType));
}

void FenceTracker::SetPendingFenceValue(uint64_t commandQueueId, uint64_t fenceValue)
{
	m_pendingFences[commandQueueId] = fenceValue;
}