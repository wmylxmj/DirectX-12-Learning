#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueueManager.h"
#include "Resource.h"
#include "../Utilities/Hash.h"

#include <vector>
#include <queue>
#include <mutex>
#include <unordered_map>

class Device;

class LinearAllocatorPage : public Resource
{
public:
	LinearAllocatorPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES resourceState);
	~LinearAllocatorPage();

	void Map();
	void Unmap();

	using Resource::m_gpuVirtualAddress;
	void* m_cpuMemoryAddress;
};

class LinearAllocatorPageManager
{
public:
	LinearAllocatorPageManager(ID3D12Device* pDevice, D3D12_HEAP_TYPE heapType, size_t pageSize);

	std::unique_ptr<LinearAllocatorPage> CreateNewPage(size_t pageSize);

	LinearAllocatorPage* RequestGeneralPage();
	void DiscardGeneralPages(FenceTracker fenceTracker, std::vector<LinearAllocatorPage*>& pages);

	LinearAllocatorPage* RequestLargePage(size_t pageSize);
	void DiscardLargePages(FenceTracker fenceTracker, std::vector<LinearAllocatorPage*>& pages);

	size_t GetPageSize() const;

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	const D3D12_HEAP_TYPE m_kHeapType;
	const size_t m_kPageSize;

	std::mutex m_mutex;

	// 生命周期可能为程序的整个生命周期，在堆上分配内存
	std::vector<std::unique_ptr<LinearAllocatorPage>> m_pagePool;
	std::unordered_map<LinearAllocatorPage*, std::unique_ptr<LinearAllocatorPage>> m_largePagePtrMap;

	std::queue<LinearAllocatorPage*> m_availablePages;
	std::queue<std::pair<FenceTracker, LinearAllocatorPage*>> m_retiredPages;
	std::queue<std::pair<FenceTracker, LinearAllocatorPage*>> m_deletionQueue;
};

class LinearBlock {
public:

	Resource& resource;
	size_t offset;
	size_t size;
	void* dataPtr;
	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress;

	LinearBlock(Resource& resource, size_t offset, size_t size, void* dataPtr, D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress)
		: resource(resource), offset(offset), size(size), dataPtr(dataPtr), gpuVirtualAddress(gpuVirtualAddress) {
	}
};

class LinearAllocator
{
public:
	LinearAllocator(Device& device, D3D12_HEAP_TYPE heapType);

	// 对齐默认256字节
	LinearBlock Allocate(size_t size, size_t alignment = 256);
	void Deallocate(FenceTracker fenceTracker);

private:
	LinearBlock AllocateLargePage(size_t size);

	Device& m_device;

	const D3D12_HEAP_TYPE m_kHeapType;
	const size_t m_kPageSize;

	LinearAllocatorPageManager* m_pPageManager;

	std::vector<LinearAllocatorPage*> m_retiredPages;
	std::vector<LinearAllocatorPage*> m_largePageList;

	LinearAllocatorPage* m_currentPage;
	size_t m_currentOffset;
};
