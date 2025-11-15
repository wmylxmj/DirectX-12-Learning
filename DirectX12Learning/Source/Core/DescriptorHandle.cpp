#include "DescriptorHandle.h"
#include "Descriptor.h"

DescriptorHandle::DescriptorHandle()
{
	m_cpuDescriptorHandle.ptr = (D3D12_GPU_VIRTUAL_ADDRESS)-1;
	m_gpuDescriptorHandle.ptr = (D3D12_GPU_VIRTUAL_ADDRESS)-1;
}

DescriptorHandle::DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
	: m_cpuDescriptorHandle(cpuDescriptorHandle)
{
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

DescriptorHandle::operator D3D12_CPU_DESCRIPTOR_HANDLE() const
{
	return m_cpuDescriptorHandle;
}

DescriptorHandle::operator D3D12_GPU_DESCRIPTOR_HANDLE() const
{
	return m_gpuDescriptorHandle;
}

bool DescriptorHandle::IsShaderVisible() const
{
	return m_gpuDescriptorHandle.ptr != (D3D12_GPU_VIRTUAL_ADDRESS)-1;
}