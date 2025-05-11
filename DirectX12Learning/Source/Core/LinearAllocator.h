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
public:
	LinearAllocatorPageManager(D3D12_HEAP_TYPE heapType);

private:
	const D3D12_HEAP_TYPE m_kHeapType;
	std::mutex m_mutex;

	std::vector<std::unique_ptr<LinearAllocatorPage>> m_pagePool;
	std::queue<LinearAllocatorPage*> m_availablePages;
};

class LinearAllocator
{
};
