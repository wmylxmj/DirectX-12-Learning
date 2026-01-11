#pragma once

#include "PrecompiledHeader.h"

#include <unordered_map>
#include <mutex>

class Device;

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
	RootSignature(Device& device, UINT numParameters, UINT numStaticSamplers = 0);

	void Reset(UINT numParameters, UINT numStaticSamplers = 0);

	RootParameter& operator[](size_t index);
	const RootParameter& operator[](size_t index) const;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature() const;

	void CreateRootSignature(D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

	uint64_t GetDescriptorTableBitMap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const;

protected:
	Device& m_device;

	UINT m_numParameters;
	UINT m_numStaticSamplers;

	std::unique_ptr<RootParameter[]> m_parameters;
	std::unique_ptr<CD3DX12_STATIC_SAMPLER_DESC[]> m_staticSamplers;

	uint64_t m_cbvSrvUavDescriptorTableBitMap;
	uint64_t m_samplerDescriptorTableBitMap;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
};