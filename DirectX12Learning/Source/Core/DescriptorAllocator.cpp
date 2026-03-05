#include "DescriptorAllocator.h"

DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) :
	m_kDescriptorHeapType(descriptorHeapType),
	m_pCurrentDescriptorHeap(nullptr),
	m_currentDescriptorHeapOffset(0)

{
}