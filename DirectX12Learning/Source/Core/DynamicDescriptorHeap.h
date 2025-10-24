#pragma once

#include "PrecompiledHeader.h"
#include "DescriptorHeap.h"
#include "Fence.h"

#include <mutex>
#include <unordered_map>
#include <vector>
#include <queue>

class DescriptorHeapManager
{
public:
	DescriptorHeapManager(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t numDescriptorsPerHeap);

	DescriptorHeap* RequestDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> pDevice);
	void DiscardDescriptorHeaps(std::vector<DescriptorHeap*>& descriptorHeaps);

private:
	const uint32_t m_kNumDescriptorsPerHeap;
	const D3D12_DESCRIPTOR_HEAP_TYPE m_kDescriptorHeapType;

	std::mutex m_mutex;

	// 记录每个命令队列的Fence对象
	std::unordered_map<uint64_t, std::unique_ptr<Fence>> m_fenceMap;

	std::vector<std::unique_ptr<DescriptorHeap>> m_descriptorHeapPool;
	std::queue<DescriptorHeap*> m_retiredDescriptorHeaps;
	std::queue<DescriptorHeap*> m_availableDescriptorHeaps;
};

class DynamicDescriptorHeap
{
};
