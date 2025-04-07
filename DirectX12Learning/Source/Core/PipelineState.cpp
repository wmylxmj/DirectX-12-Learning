#include "PipelineState.h"

GraphicsPipelineState::GraphicsPipelineState()
{
	ZeroMemory(&m_psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_psoDesc.NodeMask = 1; // ����Ĭ��GPU
	m_psoDesc.SampleMask = 0xFFFFFFFFu; // ����������ȫ������
	m_psoDesc.SampleDesc.Count = 1;
	m_psoDesc.InputLayout.NumElements = 0; // ���벼��Ԫ������
}