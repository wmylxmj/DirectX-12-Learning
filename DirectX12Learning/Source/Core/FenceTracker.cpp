#include "FenceTracker.h"

FenceTracker::FenceTracker(Device& device) :
	m_device(device)
{
}

void FenceTracker::SetPendingFenceValue(D3D12_COMMAND_LIST_TYPE commandListType, uint64_t fenceValue)
{
	m_pendingFences[commandListType] = fenceValue;
}