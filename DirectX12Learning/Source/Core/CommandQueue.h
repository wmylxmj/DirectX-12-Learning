#pragma once

#include "PrecompiledHeader.h"
#include "wrl.h"

class CommandQueue
{
private:
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	const D3D12_COMMAND_LIST_TYPE m_kCommandListType;
};