#pragma once

#include "PrecompiledHeader.h"
#include "CommandAllocatorPool.h"
#include "CommandQueue.h"
#include "LinearAllocator.h"
#include "DynamicDescriptorHeap.h"
#include "../Utilities/Hash.h"

#include <unordered_map>
#include <mutex>

class Device
{
public:
	Device(IUnknown* pAdapter);

	ID3D12Device* GetDevice() const;

	uint64_t CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType);
	CommandQueue& GetCommandQueue(uint64_t commandQueueId);

	ID3D12CommandAllocator* RequestCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType);
	void DiscardCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, FenceTracker fenceTracker, ID3D12CommandAllocator* pCommandAllocator);

	LinearAllocatorPageManager& GetLinearAllocatorPageManager(D3D12_HEAP_TYPE heapType);

	DescriptorHeapManager& GetDescriptorHeapManager(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags);

	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc);

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;

	std::unique_ptr<CommandQueueManager> m_pCommandQueueManager;
	std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::unique_ptr<CommandAllocatorPool>> m_commandAllocatorPoolMap;

	std::mutex m_linearAllocatorPageManagerMutex;
	std::unordered_map<std::vector<uint8_t>, std::unique_ptr<LinearAllocatorPageManager>, Hash<std::vector<uint8_t>>> m_linearAllocatorPageManagerMap;

	std::mutex m_descriptorHeapManagerMutex;
	std::unordered_map<std::vector<uint8_t>, std::unique_ptr<DescriptorHeapManager>, Hash<std::vector<uint8_t>>> m_descriptorHeapManagerMap;

	std::mutex m_rootSignatureCacheMutex;
	std::unordered_map<std::vector<uint8_t>, Microsoft::WRL::ComPtr<ID3D12RootSignature>, Hash<std::vector<uint8_t>>> m_rootSignatureCache;
};
