#include "PrecompiledHeader.h"
#include "Helper.h"
#include "CommandAllocatorPool.h"

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type) : m_commandListType(type) {}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocatorPool::RequestAllocator(Microsoft::WRL::ComPtr<ID3D12Device> device, uint64_t completedFenceValue)
{
	std::lock_guard<std::mutex> lockGuard(m_allocatorMutex);

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator = nullptr;

	// ���ѹ黹�ķ������в鿴�Ƿ��п��еķ�����
	if (!m_readyAllocators.empty()) {
		std::pair<uint64_t, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>& allocatorPair = m_readyAllocators.front();
		// ��� GPU ����ɸ÷�����������
		if (allocatorPair.first <= completedFenceValue)
		{
			allocator = allocatorPair.second;
			CHECK_HRESULT(allocator->Reset());
			m_readyAllocators.pop();
		}
	}

	// ���û�п��еķ���������ô�½�һ��
	if (allocator == nullptr)
	{
		CHECK_HRESULT(device->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&allocator)));
		m_allocatorPool.push_back(allocator);
	}

	return allocator;
}

void CommandAllocatorPool::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator)
{

}



