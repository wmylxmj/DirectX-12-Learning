#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueue.h"
#include "Resource.h"

#include <vector>
#include <queue>
#include <mutex>
#include <unordered_map>

class LinearAllocatorPage : public Resource
{
public:
	void* m_cpuMemoryAddress;
	std::unordered_map<uint64_t, uint64_t> m_pendingFences;

	LinearAllocatorPage(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, D3D12_RESOURCE_STATES resourceState);
	~LinearAllocatorPage();

	void Map();
	void Unmap();
};

class LinearAllocatorPageManager
{
public:
	LinearAllocatorPageManager(D3D12_HEAP_TYPE heapType);

private:
	const D3D12_HEAP_TYPE m_kHeapType;
	std::mutex m_mutex;

	// 生命周期可能为程序的整个生命周期，在堆上分配内存
	std::vector<std::unique_ptr<LinearAllocatorPage>> m_pagePool;
	std::queue<LinearAllocatorPage*> m_availablePages;
};

class LinearAllocator
{
};
