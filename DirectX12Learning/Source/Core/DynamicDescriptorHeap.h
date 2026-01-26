#pragma once

#include "PrecompiledHeader.h"
#include "DescriptorHeap.h"
#include "FenceTracker.h"
#include "../Utilities/Hash.h"

#include <mutex>
#include <unordered_map>
#include <vector>
#include <queue>
#include <set>

class Device;
class RootSignature;

class DescriptorHeapManager
{
public:
	DescriptorHeapManager(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags, uint32_t generalDescriptorHeapSize);

	DescriptorHeap* RequestGeneralSizeDescriptorHeap();
	void DiscardGeneralSizeDescriptorHeaps(FenceTracker fenceTracker, std::vector<DescriptorHeap*>& descriptorHeaps);

	DescriptorHeap* RequestLargeSizeDescriptorHeap(uint32_t numDescriptors);
	void DiscardLargeSizeDescriptorHeaps(FenceTracker fenceTracker, std::vector<DescriptorHeap*>& descriptorHeaps);

	uint32_t GetGeneralDescriptorHeapSize() const
	{
		return m_kGeneralDescriptorHeapSize;
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	const uint32_t m_kGeneralDescriptorHeapSize;
	const D3D12_DESCRIPTOR_HEAP_TYPE m_kDescriptorHeapType;
	const D3D12_DESCRIPTOR_HEAP_FLAGS m_kDescriptorHeapFlags;

	std::mutex m_mutex;

	std::vector<std::unique_ptr<DescriptorHeap>> m_descriptorHeapPool;
	std::queue< std::pair<FenceTracker, DescriptorHeap*>> m_retiredDescriptorHeaps;
	std::queue<DescriptorHeap*> m_availableDescriptorHeaps;

	std::unordered_map<DescriptorHeap*, std::unique_ptr<DescriptorHeap>> m_largeSizeDescriptorHeapPtrMap;
	std::queue<std::pair<FenceTracker, DescriptorHeap*>> m_deletionQueue;
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
	uint32_t m_currentDescriptorHeapOffset;

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
	public:
		void MarkRange(uint32_t beginOffset, uint32_t endOffset);

		const std::set<MarkerRange>& GetMarkerRanges() const
		{
			return m_markerRangeSet;
		}

		void Clear()
		{
			m_markerRangeSet.clear();
		}

	private:
		std::set<MarkerRange> m_markerRangeSet;
	};

	// ÃèÊö·û±íÈë¿Ú
	struct DescriptorTableEntry
	{
		DescriptorTableEntry() :
			pBaseDescriptorHandle(nullptr),
			numDescriptors(0)
		{
		}

		AssignedDescriptorHandlesMarker assignedDescriptorHandlesMarker;
		D3D12_CPU_DESCRIPTOR_HANDLE* pBaseDescriptorHandle;
		uint32_t numDescriptors;
	};

	struct DescriptorHandleCache
	{
		void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, const RootSignature& rootSignature);
		void StageDescriptorHandles(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE* descriptorHandles);

		void CopyAndBindStaleDescriptorTables(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, DescriptorHandle baseDestinationDescriptorHandle, uint32_t descriptorSize, ID3D12GraphicsCommandList* pCommandList, void(__stdcall ID3D12GraphicsCommandList::* pSetDescriptorHeap)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));
		uint32_t ComputeStagedSize();
		void UnbindAllValid();

		void CopyAndBindCommittedDescriptorTables(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, DescriptorHandle baseDestinationDescriptorHandle, uint32_t descriptorSize, ID3D12GraphicsCommandList* pCommandList, void(__stdcall ID3D12GraphicsCommandList::* pSetDescriptorHeap)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));
		uint32_t ComputeCommittedSize();

		uint64_t m_rootDescriptorTablesBitMap;

		uint64_t m_staleRootDescriptorTablesBitMap;
		DescriptorTableEntry m_rootDescriptorTables[64];
		std::vector<std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]>> m_descriptorHandles;

		uint64_t m_committedRootDescriptorTablesBitMap;
		DescriptorTableEntry m_committedRootDescriptorTables[64];
		std::vector<std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]>> m_committedDescriptorHandles;
	};

	DescriptorHandleCache m_graphicsDescriptorHandleCache;
	DescriptorHandleCache m_computeDescriptorHandleCache;
};
