#include "DynamicDescriptorHeap.h"

DescriptorHeapManager::DescriptorHeapManager(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t numDescriptorsPerHeap) : m_kDescriptorHeapType(descriptorHeapType), m_kNumDescriptorsPerHeap(numDescriptorsPerHeap)
{
}

DescriptorHeap* DescriptorHeapManager::RequestDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> pDevice)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	while (!m_retiredDescriptorHeaps.empty()) {
		DescriptorHeap* pDescriptorHeap = m_retiredDescriptorHeaps.front();

		// 检测围栏是否已完成
		bool fencesCompleted = true;
		for (auto& pendingFence : pDescriptorHeap->m_pendingFences) {
			if (!m_fenceMap[pendingFence.first]->IsFenceValueCompleted(pendingFence.second)) {
				fencesCompleted = false;
				break;
			}
		}

		if (fencesCompleted) {
			m_availableDescriptorHeaps.push(m_retiredDescriptorHeaps.front());
			m_retiredDescriptorHeaps.pop();
		}
		else {
			break;
		}
	}

	DescriptorHeap* descriptorHeapPtr = nullptr;
	if (!m_availableDescriptorHeaps.empty()) {
		descriptorHeapPtr = m_availableDescriptorHeaps.front();
		descriptorHeapPtr->m_pendingFences.clear();
		m_availableDescriptorHeaps.pop();
	}
	else {
		std::unique_ptr<DescriptorHeap> descriptorHeap = std::make_unique<DescriptorHeap>(pDevice, m_kDescriptorHeapType, m_kNumDescriptorsPerHeap);
		m_descriptorHeapPool.push_back(std::move(descriptorHeap));
		descriptorHeapPtr = m_descriptorHeapPool.back().get();
	}

	return descriptorHeapPtr;
}

void DescriptorHeapManager::DiscardDescriptorHeaps(std::vector<DescriptorHeap*>& descriptorHeaps)
{
	// 在记录围栏后调用
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& descriptorHeap : descriptorHeaps) {
		m_retiredDescriptorHeaps.push(descriptorHeap);
	}
}

void DescriptorHeapManager::RecordDescriptorHeapsFence(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, const CommandQueue& commandQueue, const std::vector<DescriptorHeap*>& descriptorHeaps)
{
}