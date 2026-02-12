#include "DynamicDescriptorHeap.h"

#include "Device.h"
#include "RootSignature.h"

#include <intrin.h>

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
	m_pDescriptorHeapManager(&device.GetDescriptorHeapManager(descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)),
	m_pCurrentDescriptorHeap(nullptr),
	m_currentDescriptorHeapOffset(0)
{
}

void DynamicDescriptorHeap::ParseGraphicsRootSignature(const RootSignature& rootSignature)
{
	m_graphicsDescriptorHandleCache.ParseRootSignature(m_kDescriptorHeapType, rootSignature);
}

void DynamicDescriptorHeap::ParseComputeRootSignature(const RootSignature& rootSignature)
{
	m_computeDescriptorHandleCache.ParseRootSignature(m_kDescriptorHeapType, rootSignature);
}

void DynamicDescriptorHeap::SetGraphicsDescriptorHandles(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandles)
{
	m_graphicsDescriptorHandleCache.StageDescriptorHandles(rootParameterIndex, offset, numDescriptors, descriptorHandles);
}

void DynamicDescriptorHeap::SetComputeDescriptorHandles(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandles)
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

void DynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, const RootSignature& rootSignature)
{
	m_staleRootDescriptorTablesBitMap = 0;
	m_committedRootDescriptorTablesBitMap = 0;

	m_rootDescriptorTablesBitMap = rootSignature.GetDescriptorTableBitMap(descriptorHeapType);

	m_descriptorHandles.clear();
	m_committedDescriptorHandles.clear();

	UINT currentOffset = 0;

	uint64_t tableParameters = m_rootDescriptorTablesBitMap;
	unsigned long rootParameterIndex;

	while (_BitScanForward64(&rootParameterIndex, tableParameters))
	{
		tableParameters ^= (static_cast<uint64_t>(1) << rootParameterIndex);

		const RootParameter& rootParameter = rootSignature[rootParameterIndex];

		// 计算描述符表的大小
		UINT tableSize = 0;
		UINT tableOffset = 0;
		for (UINT i = 0; i < rootParameter.DescriptorTable.NumDescriptorRanges; ++i)
		{
			const D3D12_DESCRIPTOR_RANGE& descriptorRange = rootParameter.DescriptorTable.pDescriptorRanges[i];
			if (descriptorRange.OffsetInDescriptorsFromTableStart != D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
			{
				tableOffset = descriptorRange.OffsetInDescriptorsFromTableStart;
			}
			tableOffset += descriptorRange.NumDescriptors;
			tableSize = std::max(tableSize, tableOffset);
		}

		DescriptorTableEntry& descriptorTableEntry = m_rootDescriptorTables[rootParameterIndex];
		descriptorTableEntry.assignedDescriptorHandlesMarker.Clear();
		descriptorTableEntry.numDescriptors = tableSize;

		m_descriptorHandles.push_back(std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(tableSize));
		descriptorTableEntry.pBaseDescriptorHandle = m_descriptorHandles.back().get();

		DescriptorTableEntry& committedDescriptorTableEntry = m_committedRootDescriptorTables[rootParameterIndex];
		committedDescriptorTableEntry.assignedDescriptorHandlesMarker.Clear();
		committedDescriptorTableEntry.numDescriptors = tableSize;

		m_committedDescriptorHandles.push_back(std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(tableSize));
		committedDescriptorTableEntry.pBaseDescriptorHandle = m_committedDescriptorHandles.back().get();
	}
}

void DynamicDescriptorHeap::DescriptorHandleCache::StageDescriptorHandles(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandles)
{
	assert(((static_cast<uint64_t>(1) << rootParameterIndex) & m_rootDescriptorTablesBitMap) != 0);
	assert(offset + numDescriptors <= m_rootDescriptorTables[rootParameterIndex].numDescriptors);

	DescriptorTableEntry& descriptorTableEntry = m_rootDescriptorTables[rootParameterIndex];
	D3D12_CPU_DESCRIPTOR_HANDLE* copyDest = descriptorTableEntry.pBaseDescriptorHandle + offset;
	for (uint32_t i = 0; i < numDescriptors; ++i)
	{
		copyDest[i] = descriptorHandles[i];
	}

	descriptorTableEntry.assignedDescriptorHandlesMarker.MarkRange(offset, offset + numDescriptors);
	m_staleRootDescriptorTablesBitMap |= (static_cast<uint64_t>(1) << rootParameterIndex);
}

void DynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindStaleDescriptorTables(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, DescriptorHandle baseDestinationDescriptorHandle, uint32_t descriptorSize, ID3D12GraphicsCommandList* pCommandList, void(__stdcall ID3D12GraphicsCommandList::* pSetDescriptorHeap)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
	uint64_t staleTableParameters = m_staleRootDescriptorTablesBitMap;
	unsigned long rootParameterIndex;

	while (_BitScanForward64(&rootParameterIndex, staleTableParameters))
	{
		staleTableParameters ^= (static_cast<uint64_t>(1) << rootParameterIndex);
		m_committedRootDescriptorTablesBitMap |= (static_cast<uint64_t>(1) << rootParameterIndex);

		DescriptorTableEntry& descriptorTableEntry = m_rootDescriptorTables[rootParameterIndex];
		const auto& markerRanges = descriptorTableEntry.assignedDescriptorHandlesMarker.GetMarkerRanges();
		UINT tableSize = markerRanges.rbegin()->endOffset;

		DescriptorTableEntry& committedDescriptorTableEntry = m_committedRootDescriptorTables[rootParameterIndex];
		committedDescriptorTableEntry.assignedDescriptorHandlesMarker = descriptorTableEntry.assignedDescriptorHandlesMarker;

		UINT numDestDescriptorRanges = 0;
		std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> pDestDescriptorRangeStarts = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(markerRanges.size());
		std::unique_ptr<UINT[]> pDestDescriptorRangeSizes = std::make_unique<UINT[]>(markerRanges.size());

		UINT numSrcDescriptorRanges = 0;
		std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> pSrcDescriptorRangeStarts = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(tableSize);
		std::unique_ptr<UINT[]> pSrcDescriptorRangeSizes = std::make_unique<UINT[]>(tableSize);

		for (const auto& markerRange : markerRanges)
		{
			pDestDescriptorRangeStarts[numDestDescriptorRanges] = baseDestinationDescriptorHandle + markerRange.beginOffset * descriptorSize;
			pDestDescriptorRangeSizes[numDestDescriptorRanges] = markerRange.endOffset - markerRange.beginOffset;
			++numDestDescriptorRanges;

			for (UINT i = markerRange.beginOffset; i < markerRange.endOffset; ++i)
			{
				committedDescriptorTableEntry.pBaseDescriptorHandle[i] = descriptorTableEntry.pBaseDescriptorHandle[i];
				pSrcDescriptorRangeStarts[numSrcDescriptorRanges] = descriptorTableEntry.pBaseDescriptorHandle[i];
				pSrcDescriptorRangeSizes[numSrcDescriptorRanges] = 1;
				++numSrcDescriptorRanges;
			}
		}

		pDevice->CopyDescriptors(
			numDestDescriptorRanges,
			pDestDescriptorRangeStarts.get(),
			pDestDescriptorRangeSizes.get(),
			numSrcDescriptorRanges,
			pSrcDescriptorRangeStarts.get(),
			pSrcDescriptorRangeSizes.get(),
			descriptorHeapType
		);

		(pCommandList->*pSetDescriptorHeap)(rootParameterIndex, baseDestinationDescriptorHandle);
		baseDestinationDescriptorHandle += tableSize * descriptorSize;
	}

	m_staleRootDescriptorTablesBitMap = 0;
}

uint32_t DynamicDescriptorHeap::DescriptorHandleCache::ComputeStagedSize()
{
	uint32_t neededSize = 0;
	uint64_t staleTableParameters = m_staleRootDescriptorTablesBitMap;
	unsigned long rootParameterIndex;

	while (_BitScanForward64(&rootParameterIndex, staleTableParameters))
	{
		staleTableParameters ^= (static_cast<uint64_t>(1) << rootParameterIndex);

		neededSize += m_rootDescriptorTables[rootParameterIndex].assignedDescriptorHandlesMarker.GetMarkerRanges().rbegin()->endOffset;
	}

	return neededSize;
}

void DynamicDescriptorHeap::DescriptorHandleCache::UnbindAllValid()
{
	m_staleRootDescriptorTablesBitMap = 0;

	uint64_t tableParameters = m_rootDescriptorTablesBitMap;
	unsigned long rootParameterIndex;

	while (_BitScanForward64(&rootParameterIndex, tableParameters))
	{
		tableParameters ^= (static_cast<uint64_t>(1) << rootParameterIndex);
		if (!m_rootDescriptorTables[rootParameterIndex].assignedDescriptorHandlesMarker.GetMarkerRanges().empty())
		{
			m_staleRootDescriptorTablesBitMap |= (static_cast<uint64_t>(1) << rootParameterIndex);
		}
	}
}

void DynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindCommittedDescriptorTables(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, DescriptorHandle baseDestinationDescriptorHandle, uint32_t descriptorSize, ID3D12GraphicsCommandList* pCommandList, void(__stdcall ID3D12GraphicsCommandList::* pSetDescriptorHeap)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
	uint64_t committedTableParameters = m_committedRootDescriptorTablesBitMap;
	unsigned long rootParameterIndex;

	while (_BitScanForward64(&rootParameterIndex, committedTableParameters))
	{
		committedTableParameters ^= (static_cast<uint64_t>(1) << rootParameterIndex);

		DescriptorTableEntry& committedDescriptorTableEntry = m_committedRootDescriptorTables[rootParameterIndex];
		const auto& markerRanges = committedDescriptorTableEntry.assignedDescriptorHandlesMarker.GetMarkerRanges();
		UINT tableSize = markerRanges.rbegin()->endOffset;

		UINT numDestDescriptorRanges = 0;
		std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> pDestDescriptorRangeStarts = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(markerRanges.size());
		std::unique_ptr<UINT[]> pDestDescriptorRangeSizes = std::make_unique<UINT[]>(markerRanges.size());

		UINT numSrcDescriptorRanges = 0;
		std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> pSrcDescriptorRangeStarts = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(tableSize);
		std::unique_ptr<UINT[]> pSrcDescriptorRangeSizes = std::make_unique<UINT[]>(tableSize);

		for (const auto& markerRange : markerRanges)
		{
			pDestDescriptorRangeStarts[numDestDescriptorRanges] = baseDestinationDescriptorHandle + markerRange.beginOffset * descriptorSize;
			pDestDescriptorRangeSizes[numDestDescriptorRanges] = markerRange.endOffset - markerRange.beginOffset;
			++numDestDescriptorRanges;

			for (UINT i = markerRange.beginOffset; i < markerRange.endOffset; ++i)
			{
				pSrcDescriptorRangeStarts[numSrcDescriptorRanges] = committedDescriptorTableEntry.pBaseDescriptorHandle[i];
				pSrcDescriptorRangeSizes[numSrcDescriptorRanges] = 1;
				++numSrcDescriptorRanges;
			}
		}

		pDevice->CopyDescriptors(
			numDestDescriptorRanges,
			pDestDescriptorRangeStarts.get(),
			pDestDescriptorRangeSizes.get(),
			numSrcDescriptorRanges,
			pSrcDescriptorRangeStarts.get(),
			pSrcDescriptorRangeSizes.get(),
			descriptorHeapType
		);

		(pCommandList->*pSetDescriptorHeap)(rootParameterIndex, baseDestinationDescriptorHandle);
		baseDestinationDescriptorHandle += tableSize * descriptorSize;
	}
}

uint32_t DynamicDescriptorHeap::DescriptorHandleCache::ComputeCommittedSize()
{
	uint32_t neededSize = 0;
	uint64_t committedTableParameters = m_committedRootDescriptorTablesBitMap;
	unsigned long rootParameterIndex;

	while (_BitScanForward64(&rootParameterIndex, committedTableParameters))
	{
		committedTableParameters ^= (static_cast<uint64_t>(1) << rootParameterIndex);

		neededSize += m_committedRootDescriptorTables[rootParameterIndex].assignedDescriptorHandlesMarker.GetMarkerRanges().rbegin()->endOffset;
	}

	return neededSize;
}