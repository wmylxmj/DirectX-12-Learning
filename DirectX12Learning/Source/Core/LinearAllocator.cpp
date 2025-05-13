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

LinearAllocatorPageManager::LinearAllocatorPageManager(D3D12_HEAP_TYPE heapType) : m_kHeapType(heapType)
{
}