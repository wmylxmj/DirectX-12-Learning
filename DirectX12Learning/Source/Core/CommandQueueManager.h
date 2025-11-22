#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueue.h"

#include <unordered_map>
#include <atomic>

class CommandQueueManager
{
public:
	CommandQueueManager(ID3D12Device* pDevice);

	CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);
	bool IsFenceValueCompleted(D3D12_COMMAND_LIST_TYPE commandListType, uint64_t fenceValue);

private:
	static std::atomic_uint64_t sm_nextCommandQueueId;

	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::unique_ptr<CommandQueue>> m_commandQueueMap;
};