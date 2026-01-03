#include "FenceTracker.h"

#include "Device.h"

FenceTracker::FenceTracker(Device& device) :
	m_device(device)
{
}

void FenceTracker::SetPendingFenceValue(D3D12_COMMAND_LIST_TYPE commandListType, uint64_t fenceValue)
{
	m_pendingFences[commandListType] = fenceValue;
}

bool FenceTracker::ArePendingFencesCompleted()
{
	for (const auto& [commandListType, pendingFenceValue] : m_pendingFences)
	{
		if (!m_device.GetCommandQueue(commandListType).IsFenceValueCompleted(pendingFenceValue))
		{
			return false;
		}
	}
	return true;
}