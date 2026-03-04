#include "CommandContext.h"

CommandContext::CommandContext(Device& device, D3D12_COMMAND_LIST_TYPE commandListType) :
	m_dynamicCbvSrvUavDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
	m_dynamicSamplerDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
	m_uploadHeapLinearAllocator(device, D3D12_HEAP_TYPE_UPLOAD)

{
	m_pCommandAllocator = device.RequestCommandAllocator(commandListType);
	CHECK_HRESULT(device.GetDevice()->CreateCommandList(1, commandListType, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_pCommandList)));
}

void CommandContext::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, ID3D12DescriptorHeap* pDescriptorHeap)
{
}

ID3D12GraphicsCommandList* CommandContext::GetCommandList() const
{
	return m_pCommandList.Get();
}

void CommandContext::BindDescriptorHeaps()
{
	UINT numDescriptorHeaps = 0;
	ID3D12DescriptorHeap* pDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	for (UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		if (m_pCurrentDescriptorHeaps[i] != nullptr)
		{
			pDescriptorHeaps[numDescriptorHeaps++] = m_pCurrentDescriptorHeaps[i].Get();
		}
	}

	if (numDescriptorHeaps > 0)
	{
		m_pCommandList->SetDescriptorHeaps(numDescriptorHeaps, pDescriptorHeaps);
	}
}