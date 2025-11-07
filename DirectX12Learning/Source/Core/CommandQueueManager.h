#pragma once

#include "PrecompiledHeader.h"
#include "CommandQueue.h"

#include <unordered_map>

class CommandQueueManager
{
public:
	CommandQueueManager(ID3D12Device* pDevice);

private:

	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice;
};