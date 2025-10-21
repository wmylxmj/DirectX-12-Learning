#include "DescriptorHeap.h"

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

DescriptorHeap::DescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t numDescriptors) :
	m_kDescriptorHeapType(descriptorHeapType),
	m_kNumDescriptors(numDescriptors),
	m_kDescriptorSize(pDevice->GetDescriptorHandleIncrementSize(descriptorHeapType))
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = descriptorHeapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 1;

	CHECK_HRESULT(pDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_pDescriptorHeap)));

	m_startDescriptorHandle = DescriptorHandle(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

DescriptorHandle DescriptorHeap::operator[](uint32_t index) const
{
	return m_startDescriptorHandle + index * m_kDescriptorSize;
}