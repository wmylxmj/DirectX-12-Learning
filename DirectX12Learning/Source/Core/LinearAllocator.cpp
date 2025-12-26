#include "LinearAllocator.h"
#include "Device.h"
#include "../Math/Align.h"

LinearAllocatorPage::LinearAllocatorPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES resourceState)
{
	m_pResource = pResource;
	m_resourceState = resourceState;
	m_gpuVirtualAddress = m_pResource->GetGPUVirtualAddress();
	m_pResource->Map(0, nullptr, &m_cpuMemoryAddress);
}

LinearAllocatorPage::~LinearAllocatorPage()
{
	Unmap();
}

void LinearAllocatorPage::Map()
{
	if (m_cpuMemoryAddress == nullptr) {
		m_pResource->Map(0, nullptr, &m_cpuMemoryAddress);
	}
}

void LinearAllocatorPage::Unmap()
{
	if (m_cpuMemoryAddress != nullptr) {
		m_pResource->Unmap(0, nullptr);
		m_cpuMemoryAddress = nullptr;
	}
}

LinearAllocatorPageManager::LinearAllocatorPageManager(ID3D12Device* pDevice, D3D12_HEAP_TYPE heapType, size_t pageSize) :
	m_pDevice(pDevice),
	m_kHeapType(heapType),
	m_kPageSize(pageSize)
{
}

LinearAllocatorPage* LinearAllocatorPageManager::RequestGeneralPage()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	while (!m_retiredPages.empty() && m_retiredPages.front().first.ArePendingFencesCompleted()) {
		m_availablePages.push(m_retiredPages.front().second);
		m_retiredPages.pop();
	}

	LinearAllocatorPage* pagePtr = nullptr;
	if (!m_availablePages.empty()) {
		pagePtr = m_availablePages.front();
		m_availablePages.pop();
	}
	else {
		auto newPage = CreateNewPage(m_kPageSize);
		m_pagePool.push_back(std::move(newPage));
		pagePtr = m_pagePool.back().get();
	}

	return pagePtr;
}

void LinearAllocatorPageManager::DiscardGeneralPages(FenceTracker fenceTracker, std::vector<LinearAllocatorPage*>& pages)
{
	// 在记录围栏后调用
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto page : pages) {
		m_retiredPages.push(std::make_pair(fenceTracker, page));
	}
}

LinearAllocatorPage* LinearAllocatorPageManager::RequestLargePage(size_t pageSize)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto largePage = CreateNewPage(pageSize);
	LinearAllocatorPage* rawPtr = largePage.get();
	m_largePagePtrMap.emplace(rawPtr, std::move(largePage));

	return rawPtr;
}

void LinearAllocatorPageManager::DiscardLargePages(FenceTracker fenceTracker, std::vector<LinearAllocatorPage*>& pages)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	while (!m_deletionQueue.empty() && m_deletionQueue.front().first.ArePendingFencesCompleted()) {
		m_largePagePtrMap.erase(m_deletionQueue.front().second);
		m_deletionQueue.pop();
	}

	for (auto& page : pages) {
		page->Unmap();
		m_deletionQueue.push(std::make_pair(fenceTracker, page));
	}
}

LinearAllocator::LinearAllocator(Device& device, D3D12_HEAP_TYPE heapType) :
	m_device(device),
	m_kHeapType(heapType),
	m_kPageSize(heapType == D3D12_HEAP_TYPE_UPLOAD ? 0x200000 : 0x10000),
	m_pPageManager(&device.GetLinearAllocatorPageManager(heapType, m_kPageSize)),
	m_currentPage(nullptr),
	m_currentOffset(0)
{
}

LinearBlock LinearAllocator::Allocate(size_t size, size_t alignment)
{
	const size_t alignedSize = AlignUp(size, alignment);

	// 申请大小超过通常页，则申请大页
	if (alignedSize > m_kPageSize) {
		return AllocateLargePage(alignedSize);
	}

	// 偏移向上对齐
	m_currentOffset = AlignUp(m_currentOffset, alignment);

	// 剩余空间不够，创建新的页
	if (m_currentOffset + alignedSize > m_kPageSize) {
		assert(m_currentPage != nullptr);
		m_retiredPages.push_back(m_currentPage);
		m_currentPage = nullptr;
	}

	if (m_currentPage == nullptr)
	{
		m_currentPage = m_pPageManager->RequestGeneralPage();
		m_currentOffset = 0;
	}

	LinearBlock block(
		*m_currentPage,
		m_currentOffset,
		alignedSize,
		(uint8_t*)m_currentPage->m_cpuMemoryAddress + m_currentOffset,
		m_currentPage->m_gpuVirtualAddress + m_currentOffset);

	m_currentOffset += alignedSize;

	return block;
}

void LinearAllocator::Deallocate(FenceTracker fenceTracker)
{
	if (m_currentPage != nullptr) {
		m_retiredPages.push_back(m_currentPage);
		m_currentPage = nullptr;
		m_currentOffset = 0;
	}

	m_pPageManager->DiscardGeneralPages(fenceTracker, m_retiredPages);
	m_retiredPages.clear();

	m_pPageManager->DiscardLargePages(fenceTracker, m_largePageList);
	m_largePageList.clear();
}

LinearBlock LinearAllocator::AllocateLargePage(size_t size)
{
	LinearAllocatorPage* page = m_pPageManager->RequestLargePage(size);
	m_largePageList.push_back(page);

	LinearBlock block(
		*page,
		0,
		size,
		page->m_cpuMemoryAddress,
		page->m_gpuVirtualAddress
	);

	return block;
}