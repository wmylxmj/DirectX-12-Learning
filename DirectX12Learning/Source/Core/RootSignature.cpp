#include "RootSignature.h"

#include "Device.h"

RootSignature::RootSignature(Device& device, UINT numParameters, UINT numStaticSamplers) :
	m_device(device)
{
	Reset(numParameters, numStaticSamplers);
}

void RootSignature::Reset(UINT numParameters, UINT numStaticSamplers)
{
	if (numParameters > 0) {
		m_parameters.reset(new RootParameter[numParameters]);
	}
	else {
		m_parameters = nullptr;
	}
	m_numParameters = numParameters;

	if (numStaticSamplers > 0) {
		m_staticSamplers.reset(new CD3DX12_STATIC_SAMPLER_DESC[numStaticSamplers]);
	}
	else {
		m_staticSamplers = nullptr;
	}
	m_numStaticSamplers = numStaticSamplers;
}

RootParameter& RootSignature::operator[](size_t index)
{
	assert(index < m_numParameters);
	return m_parameters.get()[index];
}

const RootParameter& RootSignature::operator[](size_t index) const
{
	assert(index < m_numParameters);
	return m_parameters.get()[index];
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature::GetRootSignature() const
{
	return m_pRootSignature;
}

void RootSignature::CreateRootSignature(D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.NumParameters = m_numParameters;
	rootSignatureDesc.pParameters = (const D3D12_ROOT_PARAMETER*)m_parameters.get();
	rootSignatureDesc.NumStaticSamplers = m_numStaticSamplers;
	rootSignatureDesc.pStaticSamplers = (const D3D12_STATIC_SAMPLER_DESC*)m_staticSamplers.get();
	rootSignatureDesc.Flags = flags;

	m_cbvSrvUavDescriptorTableBitMap = 0;
	m_samplerDescriptorTableBitMap = 0;

	for (UINT i = 0; i < m_numParameters; ++i) {
		const D3D12_ROOT_PARAMETER& parameter = rootSignatureDesc.pParameters[i];
		m_descriptorTableSize[i] = 0;

		if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
			assert(parameter.DescriptorTable.pDescriptorRanges != nullptr);

			// ±ê¼ÇÎ»Í¼
			if (parameter.DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
				m_samplerDescriptorTableBitMap |= ((uint64_t)1 << i);
			}
			else
			{
				m_cbvSrvUavDescriptorTableBitMap |= ((uint64_t)1 << i);
			}

			for (UINT j = 0; j < parameter.DescriptorTable.NumDescriptorRanges; ++j) {
				m_descriptorTableSize[i] += parameter.DescriptorTable.pDescriptorRanges[j].NumDescriptors;
			}
		}
	}

	m_pRootSignature = m_device.CreateRootSignature(rootSignatureDesc);
}

uint64_t RootSignature::GetDescriptorTableBitMap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const
{
	if (descriptorHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
		return m_cbvSrvUavDescriptorTableBitMap;
	}
	assert(descriptorHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	return m_samplerDescriptorTableBitMap;
}

uint32_t RootSignature::GetDescriptorTableSize(size_t rootParameterIndex) const
{
	return m_descriptorTableSize[rootParameterIndex];
}