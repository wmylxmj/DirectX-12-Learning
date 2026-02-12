#pragma once

#include "PrecompiledHeader.h"

class CommandContext
{
private:
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
};