#pragma once

#include "PrecompiledHeader.h"
#include "DescriptorHandle.h"
#include <unordered_map>

class DescriptorHeap
{
public:
	DescriptorHeap(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t numDescriptors);

	DescriptorHandle operator[](uint32_t index) const;

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	const D3D12_DESCRIPTOR_HEAP_TYPE m_kDescriptorHeapType;
	const uint32_t m_kNumDescriptors;
	const uint32_t m_kDescriptorSize;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;

	DescriptorHandle m_startDescriptorHandle;
};
