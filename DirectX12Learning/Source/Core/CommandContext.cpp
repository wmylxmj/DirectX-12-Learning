#include "CommandContext.h"

CommandContext::CommandContext(Device& device, D3D12_COMMAND_LIST_TYPE commandListType) :
	m_dynamicCbvSrvUavDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
	m_dynamicSamplerDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
{
}