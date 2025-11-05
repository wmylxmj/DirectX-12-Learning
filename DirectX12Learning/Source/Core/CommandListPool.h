#pragma once

#include "PrecompiledHeader.h"

#include <vector>
#include <queue>
#include <mutex>

class CommandListPool
{
public:
	CommandListPool(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE commandListType);

	ID3D12GraphicsCommandList* RequestCommandList(ID3D12CommandAllocator* pCommandAllocator);
	void DiscardCommandList(ID3D12GraphicsCommandList* pCommandList);

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;

	std::queue<ID3D12GraphicsCommandList*> m_availableCommandLists;
	std::mutex m_poolMutex;
};
