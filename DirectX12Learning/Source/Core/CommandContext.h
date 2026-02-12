#pragma once

#include "PrecompiledHeader.h"
#include "Device.h"

class CommandContext
{
public:
	CommandContext(Device& device, D3D12_COMMAND_LIST_TYPE commandListType);

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
};