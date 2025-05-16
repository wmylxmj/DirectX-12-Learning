#include "Fence.h"

Fence::Fence(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, uint64_t initialValue) :
	m_fenceValue(initialValue),
	m_completedFenceValueCache(initialValue)
{
	CHECK_HRESULT(pDevice->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
}

uint64_t Fence::IncreaseFenceValue(Microsoft::WRL::ComPtr<ID3D12CommandQueue> pCommandQueue)
{
	std::lock_guard<std::mutex> lock(m_fenceMutex);
	m_fenceValue++;
	CHECK_HRESULT(pCommandQueue->Signal(m_pFence.Get(), m_fenceValue));
	return m_fenceValue;
}

bool Fence::IsFenceValueCompleted(uint64_t fenceValue)
{
	// 避免查询过于频繁
	if (fenceValue > m_completedFenceValueCache) {
		m_completedFenceValueCache = std::max(m_completedFenceValueCache, m_pFence->GetCompletedValue());
	}
	return fenceValue <= m_completedFenceValueCache;
}

Microsoft::WRL::ComPtr<ID3D12Fence> Fence::GetFence()
{
	return m_pFence;
}

uint64_t Fence::GetFenceValue() const
{
	return m_fenceValue;
}