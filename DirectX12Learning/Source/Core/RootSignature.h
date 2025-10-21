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
protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
};