#pragma once

#include "PrecompiledHeader.h"

#include <mutex>

class CommandQueue
{
public:
	CommandQueue(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE commandListType);

private:
};