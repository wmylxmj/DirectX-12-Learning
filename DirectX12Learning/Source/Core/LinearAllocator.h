#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueue.h"
#include "Resource.h"

#include <vector>
#include <queue>
#include <mutex>

class LinearAllocatorPage : public Resource
{
public:

	void* m_cpuMemoryAddress;
};

class LinearAllocatorPageManager
{
};

class LinearAllocator
{
};
