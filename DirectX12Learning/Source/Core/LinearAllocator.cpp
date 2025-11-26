#include "LinearAllocator.h"
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

std::unique_ptr<LinearAllocatorPage> LinearAllocatorPageManager::CreateNewPage(size_t pageSize)
{
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Width = pageSize;

	D3D12_RESOURCE_STATES resourceState;

	if (m_kHeapType == D3D12_HEAP_TYPE_DEFAULT) {
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		resourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	else if (m_kHeapType == D3D12_HEAP_TYPE_UPLOAD) {
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}
	else if (m_kHeapType == D3D12_HEAP_TYPE_READBACK) {
		heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
	}
	else {
		assert(false);
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> pResource;
	CHECK_HRESULT(m_pDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		resourceState,
		nullptr,
		IID_PPV_ARGS(&pResource)));

	return std::make_unique<LinearAllocatorPage>(pResource, resourceState);
}

LinearAllocatorPage* LinearAllocatorPageManager::RequestGeneralPage()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	while (!m_retiredPages.empty()) {
		std::pair<FenceTracker, LinearAllocatorPage*>& pair = m_retiredPages.front();

		if (pair.first.ArePendingFencesCompleted())
		{
			m_availablePages.push(pair.second);
			m_retiredPages.pop();
		}
		else {
			break;
		}
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

	while (!m_deletionQueue.empty()) {
		std::pair<FenceTracker, LinearAllocatorPage*>& pair = m_deletionQueue.front();

		if (pair.first.ArePendingFencesCompleted())
		{
			m_largePagePtrMap.erase(pair.second);
			m_deletionQueue.pop();
		}
		else {
			break;
		}
	}

	for (auto& page : pages) {
		page->Unmap();
		m_deletionQueue.push(std::make_pair(fenceTracker, page));
	}
}

std::unordered_map<std::vector<uint8_t>, std::unique_ptr<LinearAllocatorPageManager>, Hash<std::vector<uint8_t>>> LinearAllocator::sm_pageManagerMap;

const std::unordered_map<D3D12_HEAP_TYPE, size_t> LinearAllocator::sm_kPageSizeMap = {
{ D3D12_HEAP_TYPE_DEFAULT, 0x10000 }, // 64KB
{ D3D12_HEAP_TYPE_UPLOAD, 0x200000 }, // 2MB
{ D3D12_HEAP_TYPE_READBACK, 0x10000 }, // 64KB
};

LinearAllocator::LinearAllocator(ID3D12Device* pDevice, D3D12_HEAP_TYPE heapType) :
	m_pDevice(pDevice),
	m_kHeapType(heapType),
	m_kPageSize(sm_kPageSizeMap.at(heapType)),
	m_currentPage(nullptr),
	m_currentOffset(0)
{
	LUID deviceLuid = pDevice->GetAdapterLuid();

	// 页管理器键
	std::vector<uint8_t> pageManagerKey(
		reinterpret_cast<const uint8_t*>(&deviceLuid),
		reinterpret_cast<const uint8_t*>(&deviceLuid) + sizeof(LUID)
	);

	pageManagerKey.insert(
		pageManagerKey.end(),
		reinterpret_cast<const uint8_t*>(&heapType),
		reinterpret_cast<const uint8_t*>(&heapType) + sizeof(D3D12_HEAP_TYPE)
	);

	if (!sm_pageManagerMap.contains(pageManagerKey))
	{
		sm_pageManagerMap.emplace(pageManagerKey, std::make_unique<LinearAllocatorPageManager>(m_pDevice, heapType, sm_kPageSizeMap.at(heapType)));
	}

	m_pPageManager = sm_pageManagerMap.at(pageManagerKey).get();
}

void LinearAllocator::Deallocate() {
	if (m_currentPage != nullptr) {
		m_retiredPages.push_back(m_currentPage);
		m_currentPage = nullptr;
		m_currentOffset = 0;
	}

	sm_pageManagerMap[m_kHeapType]->DiscardGeneralPages(m_retiredPages);
	m_retiredPages.clear();

	sm_pageManagerMap[m_kHeapType]->DiscardLargePages(m_largePageList);
	m_largePageList.clear();
}

LinearBlock LinearAllocator::AllocateLargePage(size_t size)
{
	LinearAllocatorPage* page = sm_pageManagerMap[m_kHeapType]->RequestLargePage(size);
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

LinearBlock LinearAllocator::Allocate(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, size_t size, size_t alignment)
{
	const size_t alignedSize = AlignUp(size, alignment);

	// 申请大小超过通常页，则申请大页
	if (alignedSize > m_kPageSize) {
		return AllocateLargePage(pDevice, alignedSize);
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
		m_currentPage = sm_pageManagerMap[m_kHeapType]->RequestGeneralPage(pDevice);
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