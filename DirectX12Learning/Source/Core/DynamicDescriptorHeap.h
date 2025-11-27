#pragma once

#include "PrecompiledHeader.h"
#include "DescriptorHeap.h"
#include "CommandQueueManager.h"

#include <mutex>
#include <unordered_map>
#include <vector>
#include <queue>

class DescriptorHeapManager
{
public:
	DescriptorHeapManager(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t numDescriptorsPerHeap);

	DescriptorHeap* RequestDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> pDevice);
	void DiscardDescriptorHeaps(std::vector<DescriptorHeap*>& descriptorHeaps);

	void RecordDescriptorHeapsFence(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, const CommandQueue& commandQueue, const std::vector<DescriptorHeap*>& descriptorHeaps);

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	const uint32_t m_kNumDescriptorsPerHeap;
	const D3D12_DESCRIPTOR_HEAP_TYPE m_kDescriptorHeapType;

	std::mutex m_mutex;

	std::vector<std::unique_ptr<DescriptorHeap>> m_descriptorHeapPool;
	std::queue< std::pair<FenceTracker, DescriptorHeap*>> m_retiredDescriptorHeaps;
	std::queue<DescriptorHeap*> m_availableDescriptorHeaps;
};

class DynamicDescriptorHeap
{
};
