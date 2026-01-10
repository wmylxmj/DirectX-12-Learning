#include "DynamicDescriptorHeap.h"

#include "Device.h"

DescriptorHeapManager::DescriptorHeapManager(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags, uint32_t generalDescriptorHeapSize) :
	m_pDevice(pDevice),
	m_kDescriptorHeapType(descriptorHeapType),
	m_kDescriptorHeapFlags(descriptorHeapFlags),
	m_kGeneralDescriptorHeapSize(generalDescriptorHeapSize)
{
}

DescriptorHeap* DescriptorHeapManager::RequestGeneralSizeDescriptorHeap()
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
		std::unique_ptr<DescriptorHeap> descriptorHeap = std::make_unique<DescriptorHeap>(m_pDevice.Get(), m_kDescriptorHeapType, m_kGeneralDescriptorHeapSize, m_kDescriptorHeapFlags);
		m_descriptorHeapPool.push_back(std::move(descriptorHeap));
		descriptorHeapPtr = m_descriptorHeapPool.back().get();
	}

	return descriptorHeapPtr;
}

void DescriptorHeapManager::DiscardGeneralSizeDescriptorHeaps(FenceTracker fenceTracker, std::vector<DescriptorHeap*>& descriptorHeaps)
{
	// 在记录围栏后调用
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& descriptorHeap : descriptorHeaps) {
		m_retiredDescriptorHeaps.push(std::make_pair(fenceTracker, descriptorHeap));
	}
}

DescriptorHeap* DescriptorHeapManager::RequestLargeSizeDescriptorHeap(uint32_t numDescriptors)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto descriptorHeap = std::make_unique<DescriptorHeap>(m_pDevice.Get(), m_kDescriptorHeapType, numDescriptors, m_kDescriptorHeapFlags);
	DescriptorHeap* rawPtr = descriptorHeap.get();
	m_largeSizeDescriptorHeapPtrMap.emplace(rawPtr, std::move(descriptorHeap));

	return rawPtr;
}

void DescriptorHeapManager::DiscardLargeSizeDescriptorHeaps(FenceTracker fenceTracker, std::vector<DescriptorHeap*>& descriptorHeaps)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	while (!m_deletionQueue.empty() && m_deletionQueue.front().first.ArePendingFencesCompleted())
	{
		m_largeSizeDescriptorHeapPtrMap.erase(m_deletionQueue.front().second);
		m_deletionQueue.pop();
	}

	for (auto& descriptorHeap : descriptorHeaps)
	{
		m_deletionQueue.push(std::make_pair(fenceTracker, descriptorHeap));
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

	for (auto it = m_markerRangeSet.begin(); it != m_markerRangeSet.end();) {
		// 如果新的区间和当前区间有重叠，则将两个区间合并
		if (!(newMarkerRange.endOffset < it->beginOffset || newMarkerRange.beginOffset > it->endOffset))
		{
			newMarkerRange.beginOffset = std::min(newMarkerRange.beginOffset, it->beginOffset);
			newMarkerRange.endOffset = std::max(newMarkerRange.endOffset, it->endOffset);
			it = m_markerRangeSet.erase(it);
		}
		else
		{
			++it;
		}
	}

	m_markerRangeSet.insert(newMarkerRange);
}