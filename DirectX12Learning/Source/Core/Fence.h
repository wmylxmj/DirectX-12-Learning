#pragma once

#include "PrecompiledHeader.h"

#include <mutex>

class Fence
{
private:
	std::mutex m_fenceMutex;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence;
	uint64_t m_fenceValue;
	uint64_t m_completedFenceValueCache;

public:
	Fence() = default;
	Fence(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, uint64_t initialValue = 0);
	Fence(Fence&& other) noexcept;

	Fence& operator=(Fence&& other) noexcept;

	uint64_t IncreaseFenceValue(Microsoft::WRL::ComPtr<ID3D12CommandQueue> pCommandQueue);
	bool IsFenceValueCompleted(uint64_t fenceValue);

	Microsoft::WRL::ComPtr<ID3D12Fence> GetFence();
	uint64_t GetFenceValue() const;
};