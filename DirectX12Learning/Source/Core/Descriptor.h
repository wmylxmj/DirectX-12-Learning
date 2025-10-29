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

class Descriptor
{
public:
	std::unordered_map<uint64_t, uint64_t> m_pendingFences;

	DescriptorHandle m_descriptorHandle;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;

	size_t m_offset;
};

class DescriptorArray
{
};
