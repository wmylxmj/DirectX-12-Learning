#include "LinearAllocator.h"

LinearAllocatorPage::LinearAllocatorPage(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, D3D12_RESOURCE_STATES resourceState) : Resource()
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

LinearAllocatorPageManager::LinearAllocatorPageManager(D3D12_HEAP_TYPE heapType, size_t pageSize) : m_kHeapType(heapType), m_kPageSize(pageSize) {}

std::unique_ptr<LinearAllocatorPage> LinearAllocatorPageManager::CreateNewPage(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, size_t pageSize)
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
	CHECK_HRESULT(pDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		resourceState,
		nullptr,
		IID_PPV_ARGS(&pResource)));

	return std::make_unique<LinearAllocatorPage>(pResource, resourceState);
}

LinearAllocatorPage* LinearAllocatorPageManager::RequestGeneralPage(Microsoft::WRL::ComPtr<ID3D12Device> pDevice)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	while (!m_retiredPages.empty()) {
		auto page = m_retiredPages.front();

		// ���Χ���Ƿ������
		bool fencesCompleted = true;
		for (auto& pendingFence : page->m_pendingFences) {
			if (!m_fenceMap[pendingFence.first]->IsFenceValueCompleted(pendingFence.second)) {
				fencesCompleted = false;
				break;
			}
		}

		if (fencesCompleted) {
			m_availablePages.push(m_retiredPages.front());
			m_retiredPages.pop();
		}
		else {
			break;
		}
	}

	LinearAllocatorPage* pagePtr = nullptr;
	if (!m_availablePages.empty()) {
		pagePtr = m_availablePages.front();
		pagePtr->m_pendingFences.clear();
		m_availablePages.pop();
	}
	else {
		auto newPage = CreateNewPage(pDevice, m_kPageSize);
		m_pagePool.push_back(std::move(newPage));
		pagePtr = m_pagePool.back().get();
	}

	return pagePtr;
}

void LinearAllocatorPageManager::RecordPagesFence(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, const CommandQueue& commandQueue, const std::vector<LinearAllocatorPage*>& pages)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (!m_fenceMap.contains(commandQueue.GetNonReusableId())) {
		m_fenceMap.emplace(commandQueue.GetNonReusableId(), std::make_unique<Fence>(pDevice));
	}

	m_fenceMap[commandQueue.GetNonReusableId()]->IncreaseFenceValue(commandQueue.GetCommandQueue());

	for (auto page : pages) {
		page->m_pendingFences[commandQueue.GetNonReusableId()] = m_fenceMap[commandQueue.GetNonReusableId()]->GetFenceValue();
	}
}