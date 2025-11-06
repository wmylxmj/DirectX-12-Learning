#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueue.h"

class CommandQueueManager
{
public:
	CommandQueueManager(Microsoft::WRL::ComPtr<ID3D12Device> pDevice);
	~CommandQueueManager();

	void ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList);
	void ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList, const CommandQueue& queue);
	void ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList, uint64_t& fenceValue, const CommandQueue& queue);
	void ExecuteCommandList()
}