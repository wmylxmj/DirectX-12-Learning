#include "PrecompiledHeader.h"
#include "CommandAllocatorPool.h"

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type) : m_kCommandListType(type) {}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocatorPool::RequestCommandAllocator(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, uint64_t completedFenceValue)
{
	// ��������ȷ���̰߳�ȫ
	std::lock_guard<std::mutex> lockGuard(m_allocatorMutex);

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pAllocator = nullptr;

	// ���ѹ黹�ķ������в鿴�Ƿ��п��еķ�����
	if (!m_readyAllocators.empty()) {
		std::pair<uint64_t, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>& allocatorPair = m_readyAllocators.front();
		// ��� GPU ����ɸ÷�����������
		if (allocatorPair.first <= completedFenceValue)
		{
			pAllocator = allocatorPair.second;
			CHECK_HRESULT(pAllocator->Reset());
			m_readyAllocators.pop();
		}
	}

	// ���û�п��еķ���������ô�½�һ��
	if (pAllocator == nullptr)
	{
		CHECK_HRESULT(pDevice->CreateCommandAllocator(m_kCommandListType, IID_PPV_ARGS(&pAllocator)));
	}

	return pAllocator;
}

void CommandAllocatorPool::DiscardCommandAllocator(uint64_t fenceValueForReset, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> pCommandAllocator)
{
	// ��������ȷ���̰߳�ȫ
	std::lock_guard<std::mutex> lockGuard(m_allocatorMutex);
	// �黹������
	m_readyAllocators.push(std::make_pair(fenceValueForReset, pCommandAllocator));
}