#pragma once

#include "PrecompiledHeader.h"
#include "CommandAllocatorPool.h"

#include <unordered_map>

class Device
{
public:
	Device(IUnknown* pAdapter);

	ID3D12CommandAllocator* RequestCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType);
	{
		return m_commandAllocatorPoolMap[commandListType]->RequestCommandAllocator();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;

	std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::unique_ptr<CommandAllocatorPool>> m_commandAllocatorPoolMap;
};