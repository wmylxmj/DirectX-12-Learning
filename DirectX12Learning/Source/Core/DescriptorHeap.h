#pragma once

#include "PrecompiledHeader.h"
#include "DescriptorHandle.h"
#include <unordered_map>

#include <unordered_map>

class DescriptorHandle
{
public:
	DescriptorHandle();
	DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle);
	DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle);

	void operator +=(int offsetScaledByDescriptorSize);
	DescriptorHandle operator +(int offsetScaledByDescriptorSize) const;

	operator D3D12_CPU_DESCRIPTOR_HANDLE() const;
	operator D3D12_GPU_DESCRIPTOR_HANDLE() const;

	bool IsShaderVisible() const;
	bool IsNull() const;

private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuDescriptorHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuDescriptorHandle;
};

class DescriptorHeap
{
public:
	DescriptorHeap(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags);

	DescriptorHandle operator[](uint32_t index) const;

private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
	const D3D12_DESCRIPTOR_HEAP_TYPE m_kDescriptorHeapType;
	const uint32_t m_kNumDescriptors;
	const uint32_t m_kDescriptorSize;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;

	DescriptorHandle m_startDescriptorHandle;
};
