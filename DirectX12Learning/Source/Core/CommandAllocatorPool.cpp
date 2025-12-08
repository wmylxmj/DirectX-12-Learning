#include "CommandAllocatorPool.h"

CommandAllocatorPool::CommandAllocatorPool(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE commandListType) :
	m_pDevice(pDevice),
	m_kCommandListType(commandListType)
{
}

ID3D12CommandAllocator* CommandAllocatorPool::RequestCommandAllocator()
{
	// 互斥锁，确保线程安全
	std::lock_guard<std::mutex> lockGuard(m_poolMutex);

	ID3D12CommandAllocator* pAllocator = nullptr;

	// 在已归还的分配器中查看是否有围栏已完成的分配器
	while (!m_retiredCommandAllocators.empty() && m_retiredCommandAllocators.front().first.ArePendingFencesCompleted())
	{
		m_availableCommandAllocators.push(m_retiredCommandAllocators.front().second);
		m_retiredCommandAllocators.pop();
	}

	// 查看是否有空闲的分配器
	if (!m_availableCommandAllocators.empty()) {
		pAllocator = m_availableCommandAllocators.front();
		CHECK_HRESULT(pAllocator->Reset());
		m_availableCommandAllocators.pop();
	}

	// 如果没有空闲的分配器，那么新建一个
	if (pAllocator == nullptr)
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pNewAllocator;
		CHECK_HRESULT(m_pDevice->CreateCommandAllocator(m_kCommandListType, IID_PPV_ARGS(&pNewAllocator)));
		m_commandAllocatorPool.push_back(Microsoft::WRL::ComPtr<ID3D12CommandAllocator>(pNewAllocator));
		pAllocator = pNewAllocator.Get();
	}

	return pAllocator;
}

void CommandAllocatorPool::DiscardCommandAllocator(FenceTracker fenceTracker, ID3D12CommandAllocator* pCommandAllocator)
{
	// 互斥锁，确保线程安全
	std::lock_guard<std::mutex> lockGuard(m_poolMutex);
	// 归还分配器
	m_retiredCommandAllocators.push(std::make_pair(fenceTracker, pCommandAllocator));
}