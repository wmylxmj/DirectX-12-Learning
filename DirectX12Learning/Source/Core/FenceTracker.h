#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueueManager.h"

#include <unordered_map>

class FenceTracker
{
public:
	FenceTracker(CommandQueueManager& commandQueueManager);

	void SetPendingFenceValue(D3D12_COMMAND_LIST_TYPE commandListType, uint64_t fenceValue);

	bool ArePendingFencesCompleted();

private:

	CommandQueueManager& m_commandQueueManager;
	std::unordered_map<D3D12_COMMAND_LIST_TYPE, uint64_t> m_pendingFences;
};