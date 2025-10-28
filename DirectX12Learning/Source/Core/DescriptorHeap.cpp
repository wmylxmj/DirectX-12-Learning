#include "DescriptorHeap.h"

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