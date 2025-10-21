#pragma once

#include "PrecompiledHeader.h"
#include <unordered_map>

class DescriptorHandle
{
public:
	DescriptorHandle();
	DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle);

	void operator +=(int offsetScaledByDescriptorSize);
	DescriptorHandle operator +(int offsetScaledByDescriptorSize) const;

private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuDescriptorHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuDescriptorHandle;
};

class DescriptorHeap
{
public:
	DescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t numDescriptors);

	DescriptorHandle operator[](uint32_t index) const;

private:
	const D3D12_DESCRIPTOR_HEAP_TYPE m_kDescriptorHeapType;
	const uint32_t m_kNumDescriptors;
	const uint32_t m_kDescriptorSize;

	std::unordered_map<uint64_t, uint64_t> m_pendingFences;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;

	DescriptorHandle m_startDescriptorHandle;
};
