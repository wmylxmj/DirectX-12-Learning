#pragma once

#include "PrecompiledHeader.h"
#include "DescriptorHeap.h"
#include "CommandQueueManager.h"
#include "../Utilities/Hash.h"

#include <mutex>
#include <unordered_map>
#include <vector>
#include <queue>

class Device;

class DescriptorHeapManager
{
public:
	DescriptorHeapManager(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t numDescriptorsPerHeap, D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags);

	DescriptorHeap* RequestDescriptorHeap();
	void DiscardDescriptorHeaps(FenceTracker fenceTracker, std::vector<DescriptorHeap*>& descriptorHeaps);

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	const uint32_t m_kNumDescriptorsPerHeap;
	const D3D12_DESCRIPTOR_HEAP_TYPE m_kDescriptorHeapType;
	const D3D12_DESCRIPTOR_HEAP_FLAGS m_kDescriptorHeapFlags;

	std::mutex m_mutex;
	std::vector<std::unique_ptr<DescriptorHeap>> m_descriptorHeapPool;
	std::queue< std::pair<FenceTracker, DescriptorHeap*>> m_retiredDescriptorHeaps;
	std::queue<DescriptorHeap*> m_availableDescriptorHeaps;
};

class DynamicDescriptorHeap
{
public:
	DynamicDescriptorHeap(Device& device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType);

private:
	Device& m_device;
	const D3D12_DESCRIPTOR_HEAP_TYPE m_kDescriptorHeapType;
	DescriptorHeapManager* m_pDescriptorHeapManager;
};
