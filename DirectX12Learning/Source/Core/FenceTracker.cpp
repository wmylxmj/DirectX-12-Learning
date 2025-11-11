#include "FenceTracker.h"

FenceTracker::FenceTracker(CommandQueueManager& commandQueueManager)
	: m_commandQueueManager(commandQueueManager)
{
}

void FenceTracker::SetPendingFenceValue(D3D12_COMMAND_LIST_TYPE commandListType, uint64_t fenceValue)
{
}