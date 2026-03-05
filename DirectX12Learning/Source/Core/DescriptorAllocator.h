#pragma once

#include "PrecompiledHeader.h"
#include "DescriptorHeap.h"

#include <mutex>
#include <vector>

class DescriptorAllocator
{
public:
	DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType);

private:
	D3D12_DESCRIPTOR_HEAP_TYPE m_kDescriptorHeapType;

	std::mutex m_mutex;
	std::vector<std::unique_ptr<DescriptorHeap>> m_descriptorHeapPool;

	DescriptorHeap* m_pCurrentDescriptorHeap;
	uint32_t m_currentDescriptorHeapOffset;
};
