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
};