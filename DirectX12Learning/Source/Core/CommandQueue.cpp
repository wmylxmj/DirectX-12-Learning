#include "CommandQueue.h"

CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, D3D12_COMMAND_LIST_TYPE commandListType) : m_kCommandListType(commandListType)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = commandListType;
	queueDesc.NodeMask = 1;
	CHECK_HRESULT(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue)));

	CHECK_HRESULT(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
	m_fenceValue = 0;
}