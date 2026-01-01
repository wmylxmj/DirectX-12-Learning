#include "CommandQueue.h"

CommandQueue::CommandQueue(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE commandListType) :
	m_pDevice(pDevice),
	m_kCommandListType(commandListType),
	m_fenceValue(0),
	m_completedFenceValueCache(0)
{
	// 눼쉔츱즈뚠죗
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = commandListType;
	queueDesc.NodeMask = 1;
	CHECK_HRESULT(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue)));

	// 눼쉔鍋으
	CHECK_HRESULT(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
}