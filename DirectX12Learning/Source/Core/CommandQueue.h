#pragma once

#include "PrecompiledHeader.h"
#include "wrl.h"

class CommandQueue
{
private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;

	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	uint64_t m_fenceValue;

public:
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;

	CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, D3D12_COMMAND_LIST_TYPE commandListType);
};