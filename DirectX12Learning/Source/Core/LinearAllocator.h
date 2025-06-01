#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueue.h"
#include "Resource.h"
#include "Fence.h"

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
	LinearAllocatorPageManager(D3D12_HEAP_TYPE heapType, size_t pageSize);

	std::unique_ptr<LinearAllocatorPage> CreateNewPage(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, size_t pageSize);
	LinearAllocatorPage* RequestGeneralPage(Microsoft::WRL::ComPtr<ID3D12Device> pDevice);

	void RecordPagesFence(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, const CommandQueue& commandQueue, const std::vector<LinearAllocatorPage*>& pages);

private:
	const D3D12_HEAP_TYPE m_kHeapType;
	const size_t m_kPageSize;

	std::mutex m_mutex;

	// ��¼ÿ��������е�Fence����
	std::unordered_map<uint64_t, std::unique_ptr<Fence>> m_fenceMap;

	// �������ڿ���Ϊ����������������ڣ��ڶ��Ϸ����ڴ�
	std::vector<std::unique_ptr<LinearAllocatorPage>> m_pagePool;
	std::unordered_map<LinearAllocatorPage*, std::unique_ptr<LinearAllocatorPage>> m_largePagePtrMap;

	std::queue<LinearAllocatorPage*> m_availablePages;
	std::queue<LinearAllocatorPage*> m_retiredPages;
	std::queue<LinearAllocatorPage*> m_deletionQueue;
};

class LinearAllocator
{
};
