#pragma once

#include "PrecompiledHeader.h"
#include "CommandAllocatorPool.h"
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

	// 创建根签名

	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;

	std::unique_ptr<CommandQueueManager> m_pCommandQueueManager;

	std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::unique_ptr<CommandAllocatorPool>> m_commandAllocatorPoolMap;

	std::unordered_map<std::vector<uint8_t>, std::unique_ptr<LinearAllocatorPageManager>, Hash<std::vector<uint8_t>>> m_rootSignatureAllocatorMap;
};
