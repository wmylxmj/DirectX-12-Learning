#include "CommandContext.h"

CommandContext::CommandContext(Device& device, D3D12_COMMAND_LIST_TYPE commandListType) :
	m_dynamicCbvSrvUavDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
	m_dynamicSamplerDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
	m_uploadHeapLinearAllocator(device, D3D12_HEAP_TYPE_UPLOAD)

{
	m_pCommandAllocator = device.RequestCommandAllocator(commandListType);
	CHECK_HRESULT(device.GetDevice()->CreateCommandList(1, commandListType, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_pCommandList)));
}