#pragma once

#include "PrecompiledHeader.h"

class PipelineState
{
protected:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;
};

class GraphicsPipelineState : public PipelineState
{
private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_psoDesc;

public:
	GraphicsPipelineState();
};