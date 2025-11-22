#include "CommandQueueManager.h"

CommandQueueManager::CommandQueueManager(ID3D12Device* pDevice)
	: m_pDevice(pDevice)
{
	m_commandQueueMap.emplace(D3D12_COMMAND_LIST_TYPE_DIRECT, std::make_unique<CommandQueue>(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT));
	m_commandQueueMap.emplace(D3D12_COMMAND_LIST_TYPE_COMPUTE, std::make_unique<CommandQueue>(pDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE));
	m_commandQueueMap.emplace(D3D12_COMMAND_LIST_TYPE_COPY, std::make_unique<CommandQueue>(pDevice, D3D12_COMMAND_LIST_TYPE_COPY));
}

CommandQueue& CommandQueueManager::GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
	return *sm_commandQueueMap.at(m_commandQueueIdMap.at(commandListType));
}

bool CommandQueueManager::IsFenceValueCompleted(D3D12_COMMAND_LIST_TYPE commandListType, uint64_t fenceValue)
{
	return GetCommandQueue(commandListType).IsFenceValueCompleted(fenceValue);
}