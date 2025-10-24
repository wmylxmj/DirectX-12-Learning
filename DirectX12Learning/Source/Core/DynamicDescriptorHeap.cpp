#include "DynamicDescriptorHeap.h"

DescriptorHeapManager::DescriptorHeapManager(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t numDescriptorsPerHeap) : m_kDescriptorHeapType(descriptorHeapType), m_kNumDescriptorsPerHeap(numDescriptorsPerHeap)
{
}

DescriptorHeap* DescriptorHeapManager::RequestDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> pDevice)
{
	return nullptr;
}