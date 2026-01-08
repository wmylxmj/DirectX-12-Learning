#include "DynamicDescriptorHeap.h"

#include "Device.h"

DescriptorHeapManager::DescriptorHeapManager(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags, uint32_t numDescriptorsPerHeap) :
	m_pDevice(pDevice),
	m_kDescriptorHeapType(descriptorHeapType),
	m_kNumDescriptorsPerHeap(numDescriptorsPerHeap),
	m_kDescriptorHeapFlags(descriptorHeapFlags)
{
}

DescriptorHeap* DescriptorHeapManager::RequestDescriptorHeap()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	while (!m_retiredDescriptorHeaps.empty() && m_retiredDescriptorHeaps.front().first.ArePendingFencesCompleted()) {
		m_availableDescriptorHeaps.push(m_retiredDescriptorHeaps.front().second);
		m_retiredDescriptorHeaps.pop();
	}

	DescriptorHeap* descriptorHeapPtr = nullptr;
	if (!m_availableDescriptorHeaps.empty()) {
		descriptorHeapPtr = m_availableDescriptorHeaps.front();
		m_availableDescriptorHeaps.pop();
	}
	else {
		std::unique_ptr<DescriptorHeap> descriptorHeap = std::make_unique<DescriptorHeap>(m_pDevice.Get(), m_kDescriptorHeapType, m_kNumDescriptorsPerHeap, m_kDescriptorHeapFlags);
		m_descriptorHeapPool.push_back(std::move(descriptorHeap));
		descriptorHeapPtr = m_descriptorHeapPool.back().get();
	}

	return descriptorHeapPtr;
}

void DescriptorHeapManager::DiscardDescriptorHeaps(FenceTracker fenceTracker, std::vector<DescriptorHeap*>& descriptorHeaps)
{
	// 在记录围栏后调用
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& descriptorHeap : descriptorHeaps) {
		m_retiredDescriptorHeaps.push(std::make_pair(fenceTracker, descriptorHeap));
	}
}

DynamicDescriptorHeap::DynamicDescriptorHeap(Device& device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) :
	m_device(device),
	m_kDescriptorHeapType(descriptorHeapType),
	m_pDescriptorHeapManager(&device.GetDescriptorHeapManager(descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
{
}

void DynamicDescriptorHeap::AssignedDescriptorHandlesMarker::MarkRange(uint32_t beginOffset, uint32_t endOffset)
{
	MarkerRange newMarkerRange;
	newMarkerRange.beginOffset = beginOffset;
	newMarkerRange.endOffset = endOffset;
}