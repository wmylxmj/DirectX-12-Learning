#pragma once

#include "PrecompiledHeader.h"
#include "DescriptorHeap.h"
#include "FenceTracker.h"
#include "../Utilities/Hash.h"

#include <mutex>
#include <unordered_map>
#include <vector>
#include <queue>

class Device;

class DescriptorHeapManager
{
public:
	DescriptorHeapManager(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags, uint32_t numDescriptorsPerHeap);

	DescriptorHeap* RequestDescriptorHeap();
	void DiscardDescriptorHeaps(FenceTracker fenceTracker, std::vector<DescriptorHeap*>& descriptorHeaps);

	uint32_t GetNumDescriptorsPerHeap() const { return m_kNumDescriptorsPerHeap; }

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

	DescriptorHeap* m_pCurrentDescriptorHeap;

	struct MarkerRange
	{
		bool operator<(const MarkerRange& other) const
		{
			return beginOffset < other.beginOffset;
		}

		uint32_t beginOffset;
		uint32_t endOffset;
	};

	class AssignedDescriptorHandlesMarker
	{
	};

	// ÃèÊö·û±íÈë¿Ú
	struct DescriptorTableEntry
	{
		DescriptorTableEntry() :
			assignedDescriptorHandlesBitMap(0),
			pBaseDescriptorHandle(nullptr),
			numDescriptors(0)
		{
		}

		// ÒÑ·ÖÅäµÄÃèÊö·û¾ä±úÎ»Í¼
		uint64_t assignedDescriptorHandlesBitMap;

		// »ùÃèÊö·û¾ä±ú
		D3D12_CPU_DESCRIPTOR_HANDLE* pBaseDescriptorHandle;
		uint32_t numDescriptors;
	};

	struct DescriptorHandleCache
	{
	};
};
