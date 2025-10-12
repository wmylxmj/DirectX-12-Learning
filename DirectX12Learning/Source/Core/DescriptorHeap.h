#pragma once

#include "PrecompiledHeader.h"

class DescriptorHandle
{
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
