#pragma once

#include "PrecompiledHeader.h"
#include <queue>
#include <mutex>

class CommandListPool
{
private:
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;
	std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> m_availableCommandLists;
	std::mutex m_poolMutex;

public:
	CommandListPool(D3D12_COMMAND_LIST_TYPE commandListType);
};