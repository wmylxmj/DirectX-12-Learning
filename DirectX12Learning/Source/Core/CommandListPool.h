#pragma once

#include "PrecompiledHeader.h"
#include <queue>
#include <mutex>

class CommandListPool
{
public:
	CommandListPool(D3D12_COMMAND_LIST_TYPE commandListType);

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> RequestCommandList(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator);
	void DiscardCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList);

private:
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;
	std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> m_availableCommandLists;
	std::mutex m_poolMutex;
};
