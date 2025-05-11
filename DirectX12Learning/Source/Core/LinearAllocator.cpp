#include "LinearAllocator.h"

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