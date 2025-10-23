#include "CommandQueue.h"

std::atomic_uint64_t CommandQueue::sm_nextNonReusableId = 1;

CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, D3D12_COMMAND_LIST_TYPE commandListType) :
	m_kCommandListType(commandListType),
	m_kNonReusableId(sm_nextNonReusableId++),
	m_commandAllocatorPool(commandListType),
	m_commandListPool(commandListType)
{
	// 创建命令队列
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = commandListType;
	queueDesc.NodeMask = 1;
	CHECK_HRESULT(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue)));

	// 创建围栏
	m_pFence = std::make_unique<Fence>(pDevice);
}

uint64_t CommandQueue::IncrementFenceValue()
{
	std::lock_guard<std::mutex> lockGuard(m_fenceMutex);
	return m_pFence->IncrementFenceValue(m_pCommandQueue);
}

bool CommandQueue::IsFenceValueCompleted(uint64_t fenceValue)
{
	return m_pFence->IsFenceValueCompleted(fenceValue);
}

void CommandQueue::StallForAnotherQueueFence(const CommandQueue& queue, uint64_t fenceValue)
{
	m_pCommandQueue->Wait(queue.m_pFence->GetFence().Get(), fenceValue);
}

void CommandQueue::StallForAnotherQueueCompletion(const CommandQueue& queue)
{
	m_pCommandQueue->Wait(queue.m_pFence->GetFence().Get(), queue.m_pFence->GetFenceValue());
}

void CommandQueue::WaitForFence(uint64_t fenceValue)
{
	m_pFence->WaitForFenceValue(fenceValue, nullptr);
}

void CommandQueue::WaitForIdle()
{
	WaitForFence(IncrementFenceValue());
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueue() const
{
	return m_pCommandQueue;
}

uint64_t CommandQueue::GetFenceValue() const
{
	return m_pFence->GetFenceValue();
}

uint64_t CommandQueue::GetNonReusableId() const
{
	return m_kNonReusableId;
}

uint64_t CommandQueue::ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList)
{
	std::lock_guard<std::mutex> lockGuard(m_fenceMutex);

	CHECK_HRESULT(pCommandList->Close());
	m_pCommandQueue->ExecuteCommandLists(1, &RvalueToLvalue((ID3D12CommandList*)pCommandList.Get()));

	return m_pFence->IncrementFenceValue(m_pCommandQueue);
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandQueue::RequestCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device> pDevice)
{
	return m_commandAllocatorPool.RequestCommandAllocator(pDevice, m_pFence->GetFence()->GetCompletedValue());
}

void CommandQueue::DiscardCommandAllocator(uint64_t fenceValueForReset, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator)
{
	m_commandAllocatorPool.DiscardCommandAllocator(fenceValueForReset, pCommandAllocator);
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandQueue::RequestCommandList(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator)
{
	return m_commandListPool.RequestCommandList(pDevice, pCommandAllocator);
}

void CommandQueue::DiscardCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList)
{
	m_commandListPool.DiscardCommandList(pCommandList);
}