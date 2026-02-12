#pragma once

#include "PrecompiledHeader.h"
#include "Device.h"

class CommandContext
{
public:
	CommandContext(Device& device, D3D12_COMMAND_LIST_TYPE commandListType);

private:
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_currentGraphicsRootSignature;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_currentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};