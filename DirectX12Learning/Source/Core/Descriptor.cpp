#include "Descriptor.h"

DescriptorHandle::DescriptorHandle()
{
	m_cpuDescriptorHandle.ptr = (D3D12_GPU_VIRTUAL_ADDRESS)-1;
	m_gpuDescriptorHandle.ptr = (D3D12_GPU_VIRTUAL_ADDRESS)-1;
}

DescriptorHandle::DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle)
	: m_cpuDescriptorHandle(cpuDescriptorHandle), m_gpuDescriptorHandle(gpuDescriptorHandle)
{
}

void DescriptorHandle::operator+=(int offsetScaledByDescriptorSize)
{
	if (m_cpuDescriptorHandle.ptr != (D3D12_GPU_VIRTUAL_ADDRESS)-1)
	{
		m_cpuDescriptorHandle.ptr += offsetScaledByDescriptorSize;
	}
	if (m_gpuDescriptorHandle.ptr != (D3D12_GPU_VIRTUAL_ADDRESS)-1)
	{
		m_gpuDescriptorHandle.ptr += offsetScaledByDescriptorSize;
	}
}

DescriptorHandle DescriptorHandle::operator+(int offsetScaledByDescriptorSize) const
{
	DescriptorHandle ret = *this;
	ret += offsetScaledByDescriptorSize;
	return ret;
}