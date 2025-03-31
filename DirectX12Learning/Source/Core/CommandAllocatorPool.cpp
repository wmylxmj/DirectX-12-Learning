#include "PrecompiledHeader.h"
#include "CommandAllocatorPool.h"

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type) : m_kCommandListType(type) {}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocatorPool::RequestAllocator(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, uint64_t completedFenceValue)
{
	std::lock_guard<std::mutex> lockGuard(m_allocatorMutex);

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pAllocator = nullptr;

	// 在已归还的分配器中查看是否有空闲的分配器
	if (!m_readyAllocators.empty()) {
		std::pair<uint64_t, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>& allocatorPair = m_readyAllocators.front();
		// 如果 GPU 已完成该分配器的命令
		if (allocatorPair.first <= completedFenceValue)
		{
			pAllocator = allocatorPair.second;
			CHECK_HRESULT(pAllocator->Reset());
			m_readyAllocators.pop();
		}
	}

	// 如果没有空闲的分配器，那么新建一个
	if (pAllocator == nullptr)
	{
		CHECK_HRESULT(pDevice->CreateCommandAllocator(m_kCommandListType, IID_PPV_ARGS(&pAllocator)));
		m_allocatorPool.push_back(pAllocator);
	}

	return pAllocator;
}

void CommandAllocatorPool::DiscardAllocator(uint64_t fenceValue, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pAllocator)
{
	std::lock_guard<std::mutex> lockGuard(m_allocatorMutex);
	// 归还分配器
	m_readyAllocators.push(std::make_pair(fenceValue, pAllocator));
}

