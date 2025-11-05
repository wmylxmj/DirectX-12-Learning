#include "CommandListPool.h"

CommandListPool::CommandListPool(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE commandListType)
	: m_pDevice(pDevice), m_kCommandListType(commandListType)
{
}

ID3D12GraphicsCommandList* CommandListPool::RequestCommandList(ID3D12CommandAllocator* pCommandAllocator)
{
	std::lock_guard<std::mutex> lockGuard(m_poolMutex);

	ID3D12GraphicsCommandList* pCommandList;

	if (!m_availableCommandLists.empty())
	{
		pCommandList = m_availableCommandLists.front();
		m_availableCommandLists.pop();
	}
	else
	{
		CHECK_HRESULT(m_pDevice->CreateCommandList(1, m_kCommandListType, pCommandAllocator, nullptr, IID_PPV_ARGS(&pCommandList)));
		CHECK_HRESULT(pCommandList->Close());
	}

	return pCommandList;
}

void CommandListPool::DiscardCommandList(ID3D12GraphicsCommandList* pCommandList)
{
	std::lock_guard<std::mutex> lockGuard(m_poolMutex);
	m_availableCommandLists.push(pCommandList);
}