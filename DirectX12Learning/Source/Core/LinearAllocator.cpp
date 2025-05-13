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

LinearAllocatorPageManager::LinearAllocatorPageManager(D3D12_HEAP_TYPE heapType) : m_kHeapType(heapType) {}

LinearAllocatorPage* LinearAllocatorPageManager::CreateNewPage(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, size_t pageSize)
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

	{
		std::lock_guard<std::mutex> lock(m_mutex);

		LinearAllocatorPage* pPage = new LinearAllocatorPage(pResource, resourceState);
		m_pagePool.emplace_back(pPage);

		return pPage;
	}
}