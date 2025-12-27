#pragma once

#include "PrecompiledHeader.h"

#include <unordered_map>

class Device;

class FenceTracker
{
public:
	FenceTracker(Device& device);

private:
	Device& m_device;
	std::unordered_map<D3D12_COMMAND_LIST_TYPE, uint64_t> m_pendingFences;
};