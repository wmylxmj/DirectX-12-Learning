#pragma once

#include "PrecompiledHeader.h"

#include <unordered_map>

class Device;

class FenceTracker
{
public:
	FenceTracker(Device& device);
	: m_device(device)
	{
	}
	uint64_t Signal(D3D12_COMMAND_LIST_TYPE type);
	uint64_t GetCompletedFenceValue(D3D12_COMMAND_LIST_TYPE type) const;
	uint64_t GetPendingFenceValue(D3D12_COMMAND_LIST_TYPE type) const;

private:
	Device& m_device;
	std::unordered_map<D3D12_COMMAND_LIST_TYPE, uint64_t> m_pendingFences;
};