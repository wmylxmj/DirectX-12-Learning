#include "CommandQueueManager.h"

// static 变量定义
std::atomic_uint64_t CommandQueueManager::sm_nextCommandQueueId = 1;
std::mutex CommandQueueManager::sm_commandQueueMapMutex;
std::unordered_map<uint64_t, std::unique_ptr<CommandQueue>> CommandQueueManager::sm_commandQueueMap;

CommandQueueManager::CommandQueueManager(ID3D12Device* pDevice)
	: m_pDevice(pDevice)
{
	std::lock_guard<std::mutex> lockGuard(sm_commandQueueMapMutex);

	uint64_t commandQueueId = sm_nextCommandQueueId++;
	sm_commandQueueMap.emplace(commandQueueId, std::make_unique<CommandQueue>(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, commandQueueId));
	m_commandQueueIdMap.emplace(D3D12_COMMAND_LIST_TYPE_DIRECT, commandQueueId);

	commandQueueId = sm_nextCommandQueueId++;
	sm_commandQueueMap.emplace(commandQueueId, std::make_unique<CommandQueue>(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE, commandQueueId));
	m_commandQueueIdMap.emplace(D3D12_COMMAND_LIST_TYPE_COMPUTE, commandQueueId);

	commandQueueId = sm_nextCommandQueueId++;
	sm_commandQueueMap.emplace(commandQueueId, std::make_unique<CommandQueue>(pDevice, D3D12_COMMAND_LIST_TYPE_COPY, commandQueueId));
	m_commandQueueIdMap.emplace(D3D12_COMMAND_LIST_TYPE_COPY, commandQueueId);
}

uint64_t CommandQueueManager::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
	std::lock_guard<std::mutex> lockGuard(sm_commandQueueMapMutex);

	uint64_t commandQueueId = sm_nextCommandQueueId++;
}

CommandQueue& CommandQueueManager::GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
	std::lock_guard<std::mutex> lockGuard(sm_commandQueueMapMutex);
	return *sm_commandQueueMap.at(m_commandQueueIdMap.at(commandListType));
}

bool CommandQueueManager::IsFenceValueCompleted(D3D12_COMMAND_LIST_TYPE commandListType, uint64_t fenceValue)
{
	return GetCommandQueue(commandListType).IsFenceValueCompleted(fenceValue);
}