#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueueManager.h"

class FenceTracker
{
public:
	FenceTracker(CommandQueueManager* commandQueueManager);
	~FenceTracker();

	void WaitForFence(UINT64 fenceValue);
	void WaitForFence(UINT64 fenceValue, ID3D12CommandQueue* commandQueue);

	UINT64 GetFenceValue() const;
	UINT64 GetNextFenceValue() const;

private:
	void CreateFence();

	CommandQueueManager* m_commandQueueManager;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;
};