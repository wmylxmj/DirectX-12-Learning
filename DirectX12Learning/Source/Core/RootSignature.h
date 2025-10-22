#pragma once

#include "PrecompiledHeader.h"

class DescriptorRange : public CD3DX12_DESCRIPTOR_RANGE
{
public:
	using CD3DX12_DESCRIPTOR_RANGE::CD3DX12_DESCRIPTOR_RANGE;
};

class RootParameter : public CD3DX12_ROOT_PARAMETER
{
public:
	using CD3DX12_ROOT_PARAMETER::CD3DX12_ROOT_PARAMETER;
};

class RootSignature
{
public:
	RootSignature(UINT numParameters, UINT numStaticSamplers = 0);

	void Reset(UINT numParameters, UINT numStaticSamplers = 0);

protected:
	UINT m_numParameters;
	UINT m_numStaticSamplers;

	std::unique_ptr<RootParameter[]> m_parameters;
	std::unique_ptr<CD3DX12_STATIC_SAMPLER_DESC[]> m_staticSamplers;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
};