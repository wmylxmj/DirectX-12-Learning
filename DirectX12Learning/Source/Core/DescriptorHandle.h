#pragma once

#include "PrecompiledHeader.h"

#include <unordered_map>

class DescriptorHandle
{
public:
	DescriptorHandle();
	DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle);
	DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle);

	void operator +=(int offsetScaledByDescriptorSize);
	DescriptorHandle operator +(int offsetScaledByDescriptorSize) const;

	operator D3D12_CPU_DESCRIPTOR_HANDLE() const;
	operator D3D12_GPU_DESCRIPTOR_HANDLE() const;

	bool IsShaderVisible() const;
	bool IsNull() const;

private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuDescriptorHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuDescriptorHandle;
};