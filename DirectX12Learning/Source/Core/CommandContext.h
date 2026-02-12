#pragma once

#include "PrecompiledHeader.h"
#include "Device.h"
#include "DynamicDescriptorHeap.h"
#include "LinearAllocator.h"
#include "RootSignature.h"

class CommandContext
{
public:
	CommandContext(Device& device, D3D12_COMMAND_LIST_TYPE commandListType);

private:
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_pCommandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pCurrentGraphicsRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pCurrentComputeRootSignature;

	DynamicDescriptorHeap m_dynamicCbvSrvUavDescriptorHeap;
	DynamicDescriptorHeap m_dynamicSamplerDescriptorHeap;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pCurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	LinearAllocator m_uploadHeapLinearAllocator;
};