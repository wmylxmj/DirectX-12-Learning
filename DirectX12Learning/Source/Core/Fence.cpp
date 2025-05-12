#include "Fence.h"

Fence::Fence(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, uint64_t initialValue) :
	m_fenceValue(initialValue),
	m_completedFenceValueCache(initialValue)
{
	CHECK_HRESULT(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
}