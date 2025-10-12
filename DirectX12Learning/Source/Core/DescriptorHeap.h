#pragma once

#include "PrecompiledHeader.h"

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

private:
	DescriptorHandle m_startDescriptorHandle;
};
