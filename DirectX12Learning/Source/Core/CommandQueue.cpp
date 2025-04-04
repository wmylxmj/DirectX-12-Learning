#include "CommandQueue.h"

CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, D3D12_COMMAND_LIST_TYPE commandListType) :
	m_kCommandListType(commandListType),
	m_fenceValue(0),
	m_completedFenceValueCache(0),
	m_commandAllocatorPool(commandListType)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = commandListType;
	queueDesc.NodeMask = 1;
	CHECK_HRESULT(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue)));

	CHECK_HRESULT(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
}

uint64_t CommandQueue::IncreaseFenceValue()
{
	std::lock_guard<std::mutex> lockGuard(m_fenceMutex);
	m_fenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_fenceValue);
	return m_fenceValue;
}

bool CommandQueue::IsFenceValueCompleted(uint64_t fenceValue)
{
	// 避免查询过于频繁
	if (fenceValue > m_completedFenceValueCache) {
		m_completedFenceValueCache = std::max(m_completedFenceValueCache, m_pFence->GetCompletedValue());
	}
	return fenceValue <= m_completedFenceValueCache;
}

void CommandQueue::StallForAnotherQueueFence(const CommandQueue& queue, uint64_t fenceValue)
{
	m_pCommandQueue->Wait(queue.m_pFence.Get(), fenceValue);
}

void CommandQueue::StallForAnotherQueueCompletion(const CommandQueue& queue)
{
	m_pCommandQueue->Wait(queue.m_pFence.Get(), queue.m_fenceValue);
}

void CommandQueue::WaitForFence(uint64_t fenceValue)
{
	if (IsFenceValueCompleted(fenceValue)) return;

	m_pFence->SetEventOnCompletion(fenceValue, nullptr);
	m_completedFenceValueCache = fenceValue;
}

void CommandQueue::WaitForIdle()
{
	WaitForFence(IncreaseFenceValue());
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueue() const
{
	return m_pCommandQueue;
}

uint64_t CommandQueue::GetFenceValue() const
{
	return m_fenceValue;
}

uint64_t CommandQueue::ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList)
{
	std::lock_guard<std::mutex> lockGuard(m_fenceMutex);

	CHECK_HRESULT(pCommandList->Close());
	m_pCommandQueue->ExecuteCommandLists(1, &RvalueToLvalue((ID3D12CommandList*)pCommandList.Get()));

	m_fenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_fenceValue);

	return m_fenceValue;
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandQueue::RequestCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device> pDevice)
{
	return m_commandAllocatorPool.RequestAllocator(pDevice, m_pFence->GetCompletedValue());
}

void CommandQueue::DiscardCommandAllocator(uint64_t fenceValueForReset, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator)
{
	m_commandAllocatorPool.DiscardAllocator(fenceValueForReset, pCommandAllocator);
}