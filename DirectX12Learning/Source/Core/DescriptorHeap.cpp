#include "DescriptorHeap.h"

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

bool DescriptorHandle::IsNull() const
{
	return m_cpuDescriptorHandle.ptr == (D3D12_GPU_VIRTUAL_ADDRESS)-1;
}

DescriptorHeap::DescriptorHeap(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags) :
	m_pDevice(pDevice),
	m_kDescriptorHeapType(descriptorHeapType),
	m_kNumDescriptors(numDescriptors),
	m_kDescriptorSize(pDevice->GetDescriptorHandleIncrementSize(descriptorHeapType))
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = descriptorHeapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = descriptorHeapFlags;
	descriptorHeapDesc.NodeMask = 1;

	CHECK_HRESULT(pDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_pDescriptorHeap)));

	if (descriptorHeapFlags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) {
		m_startDescriptorHandle = DescriptorHandle(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}
	else
	{
		m_startDescriptorHandle = DescriptorHandle(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	}
}

DescriptorHandle DescriptorHeap::operator[](uint32_t index) const
{
	return m_startDescriptorHandle + index * m_kDescriptorSize;
}