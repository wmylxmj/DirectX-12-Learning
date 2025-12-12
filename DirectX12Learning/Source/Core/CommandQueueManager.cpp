#include "CommandQueueManager.h"

CommandQueue::CommandQueue(uint64_t commandQueueId, ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE commandListType) :
	m_kCommandQueueId(commandQueueId),
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

// static 变量定义
std::atomic_uint64_t CommandQueueManager::sm_nextCommandQueueId = 1;
std::shared_mutex CommandQueueManager::sm_commandQueueMapMutex;
std::unordered_map<uint64_t, std::unique_ptr<CommandQueue>> CommandQueueManager::sm_commandQueueMap;

CommandQueueManager::CommandQueueManager(ID3D12Device* pDevice)
	: m_pDevice(pDevice)
{
}

uint64_t CommandQueueManager::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
	std::unique_lock<std::shared_mutex> uniqueLock(sm_commandQueueMapMutex);

	uint64_t commandQueueId = sm_nextCommandQueueId++;
	sm_commandQueueMap.emplace(commandQueueId, std::unique_ptr<CommandQueue>(new CommandQueue(commandQueueId, m_pDevice.Get(), commandListType)));
	m_owningCommandQueueMap.emplace(commandQueueId, sm_commandQueueMap.at(commandQueueId).get());

	return commandQueueId;
}

CommandQueue& CommandQueueManager::GetCommandQueue(uint64_t commandQueueId)
{
	return *m_owningCommandQueueMap.at(commandQueueId);
}

void FenceTracker::SetPendingFenceValue(uint64_t commandQueueId, uint64_t fenceValue)
{
	m_pendingFences[commandQueueId] = fenceValue;
}

bool FenceTracker::ArePendingFencesCompleted()
{
	std::shared_lock<std::shared_mutex> sharedLock(CommandQueueManager::sm_commandQueueMapMutex);

	for (const auto& [commandQueueId, fenceValue] : m_pendingFences)
	{
		CommandQueue& commandQueue = *CommandQueueManager::sm_commandQueueMap.at(commandQueueId);
		if (!commandQueue.IsFenceValueCompleted(fenceValue))
		{
			return false;
		}
	}

	return true;
}