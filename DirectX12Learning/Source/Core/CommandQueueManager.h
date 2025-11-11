#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueue.h"

#include <unordered_map>

class CommandQueueManager
{
public:
	CommandQueueManager(ID3D12Device* pDevice);

	CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE type);

private:

	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::unique_ptr<CommandQueue>> m_commandQueueMap;
};