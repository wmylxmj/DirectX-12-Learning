#include "CommandQueue.h"

CommandQueue::CommandQueue(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE commandListType) :
	m_pDevice(pDevice),
	m_kCommandListType(commandListType),
	m_fenceValue(0),
	m_completedFenceValueCache(0)
{
	// 创建命令队列
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = commandListType;
	queueDesc.NodeMask = 1;
	CHECK_HRESULT(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue)));

	// 创建围栏
	CHECK_HRESULT(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
}

uint64_t CommandQueue::IncrementFenceValue()
{
	std::lock_guard<std::mutex> lockGuard(m_fenceMutex);
	m_fenceValue++;
	CHECK_HRESULT(m_pCommandQueue->Signal(m_pFence.Get(), m_fenceValue));
	return m_fenceValue;
}

bool CommandQueue::IsFenceValueCompleted(uint64_t fenceValue)
{
	// 긁출꿴璂법黨틉런
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

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
	if (IsFenceValueCompleted(fenceValue)) return;

	m_pFence->SetEventOnCompletion(fenceValue, nullptr);
	m_completedFenceValueCache = fenceValue;
}

void CommandQueue::WaitForIdle()
{
	WaitForFenceValue(IncrementFenceValue());
}

ID3D12CommandQueue* CommandQueue::GetCommandQueue() const
{
	return m_pCommandQueue.Get();
}

uint64_t CommandQueue::GetCurrentFenceValue() const
{
	return m_fenceValue;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandQueue::CreateCommandList(ID3D12CommandAllocator* pCommandAllocator)
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pNewCommandList;
	CHECK_HRESULT(m_pDevice->CreateCommandList(1, m_kCommandListType, pCommandAllocator, nullptr, IID_PPV_ARGS(&pNewCommandList)));
	CHECK_HRESULT(pNewCommandList->Close());

	return pNewCommandList;
}

uint64_t CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList* pCommandList)
{
	std::lock_guard<std::mutex> lockGuard(m_fenceMutex);

	CHECK_HRESULT(pCommandList->Close());
	m_pCommandQueue->ExecuteCommandLists(1, &RvalueToLvalue((ID3D12CommandList*)pCommandList));

	m_fenceValue++;
	CHECK_HRESULT(m_pCommandQueue->Signal(m_pFence.Get(), m_fenceValue));
	return m_fenceValue;
}

uint64_t CommandQueue::GetCommandQueueId() const
{
	return m_kCommandQueueId;
}

D3D12_COMMAND_LIST_TYPE CommandQueue::GetCommandListType() const
{
	return m_kCommandListType;
}