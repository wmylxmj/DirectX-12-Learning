#include "DescriptorHeap.h"

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