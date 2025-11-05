#include "CommandListPool.h"

CommandListPool::CommandListPool(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE commandListType)
	: m_kCommandListType(commandListType)
{
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandListPool::RequestCommandList(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator)
{
	std::lock_guard<std::mutex> lockGuard(m_poolMutex);

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList;

	if (!m_availableCommandLists.empty())
	{
		pCommandList = m_availableCommandLists.front();
		m_availableCommandLists.pop();
	}
	else
	{
		CHECK_HRESULT(pDevice->CreateCommandList(1, m_kCommandListType, pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&pCommandList)));
		CHECK_HRESULT(pCommandList->Close());
	}

	return pCommandList;
}

void CommandListPool::DiscardCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList)
{
	std::lock_guard<std::mutex> lockGuard(m_poolMutex);
	m_availableCommandLists.push(pCommandList);
}