#include "PipelineState.h"

GraphicsPipelineState::GraphicsPipelineState()
{
	ZeroMemory(&m_psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_psoDesc.NodeMask = 1; // 关联默认GPU
	m_psoDesc.SampleMask = 0xFFFFFFFFu; // 像素子样本全部保留
	m_psoDesc.SampleDesc.Count = 1;
	m_psoDesc.InputLayout.NumElements = 0; // 输入布局元素数量
}