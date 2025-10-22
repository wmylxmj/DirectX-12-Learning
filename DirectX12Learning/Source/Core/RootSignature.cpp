#include "RootSignature.h"

RootSignature::RootSignature(UINT numParameters, UINT numStaticSamplers)
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
	return m_rootSignature;
}