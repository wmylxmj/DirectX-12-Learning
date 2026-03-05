#pragma once

#include "PrecompiledHeader.h"
#include "DescriptorHeap.h"

#include <mutex>
#include <vector>

class DescriptorAllocator
{
public:

private:
	D3D12_DESCRIPTOR_HEAP_TYPE m_kDescriptorHeapType;
};
