#include "DynamicDescriptorHeap.h"

DescriptorHeapManager::DescriptorHeapManager(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t numDescriptorsPerHeap, D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags) :
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

std::unordered_map<std::vector<uint8_t>, std::unique_ptr<DescriptorHeapManager>, Hash<std::vector<uint8_t>>> DynamicDescriptorHeap::sm_descriptorHeapManagerMap;
const uint32_t DynamicDescriptorHeap::sm_kNumDescriptorsPerHeap = 1024;

DynamicDescriptorHeap::DynamicDescriptorHeap(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) :
	m_pDevice(pDevice),
	m_kDescriptorHeapType(descriptorHeapType)
{
	LUID deviceLuid = pDevice->GetAdapterLuid();

	std::vector<uint8_t> descriptorHeapManagerKey(
		reinterpret_cast<uint8_t*>(&deviceLuid),
		reinterpret_cast<uint8_t*>(&deviceLuid) + sizeof(LUID)
	);

	descriptorHeapManagerKey.insert(
		descriptorHeapManagerKey.end(),
		reinterpret_cast<uint8_t*>(&descriptorHeapType),
		reinterpret_cast<uint8_t*>(&descriptorHeapType) + sizeof(D3D12_DESCRIPTOR_HEAP_TYPE)
	);

	if (!sm_descriptorHeapManagerMap.contains(descriptorHeapManagerKey)) {
		sm_descriptorHeapManagerMap[descriptorHeapManagerKey] = std::make_unique<DescriptorHeapManager>(
			pDevice,
			descriptorHeapType,
			sm_kNumDescriptorsPerHeap,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		);
	}

	m_pDescriptorHeapManager = sm_descriptorHeapManagerMap[descriptorHeapManagerKey].get();
}