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
	return nullptr;
}