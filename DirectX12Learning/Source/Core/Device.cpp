#include "Device.h"

Device::Device(IUnknown* pAdapter)
{
	CHECK_HRESULT(D3D12CreateDevice(
		pAdapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_pDevice)
	));

	m_pCommandQueueManager = std::make_unique<CommandQueueManager>(m_pDevice.Get());
	m_pCommandQueueManager->CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

	m_commandAllocatorPoolMap.emplace(D3D12_COMMAND_LIST_TYPE_DIRECT, std::make_unique<CommandAllocatorPool>(m_pDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT));
	m_commandAllocatorPoolMap.emplace(D3D12_COMMAND_LIST_TYPE_COMPUTE, std::make_unique<CommandAllocatorPool>(m_pDevice.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE));
	m_commandAllocatorPoolMap.emplace(D3D12_COMMAND_LIST_TYPE_COPY, std::make_unique<CommandAllocatorPool>(m_pDevice.Get(), D3D12_COMMAND_LIST_TYPE_COPY));
}

ID3D12Device* Device::GetDevice() const
{
	return m_pDevice.Get();
}

uint64_t Device::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType)
{
	return m_pCommandQueueManager->CreateCommandQueue(commandListType);
}

CommandQueue& Device::GetCommandQueue(uint64_t commandQueueId)
{
	return m_pCommandQueueManager->GetCommandQueue(commandQueueId);
}

ID3D12CommandAllocator* Device::RequestCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType)
{
	return m_commandAllocatorPoolMap[commandListType]->RequestCommandAllocator();
}

void Device::DiscardCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, FenceTracker fenceTracker, ID3D12CommandAllocator* pCommandAllocator)
{
	m_commandAllocatorPoolMap[commandListType]->DiscardCommandAllocator(fenceTracker, pCommandAllocator);
}

LinearAllocatorPageManager& Device::GetLinearAllocatorPageManager(D3D12_HEAP_TYPE heapType)
{
	std::vector<uint8_t> pageManagerKey(
		reinterpret_cast<const uint8_t*>(&heapType),
		reinterpret_cast<const uint8_t*>(&heapType) + sizeof(D3D12_HEAP_TYPE)
	);

	pageManagerKey.insert(pageManagerKey.end(),
		reinterpret_cast<const uint8_t*>(&pageSize),
		reinterpret_cast<const uint8_t*>(&pageSize) + sizeof(size_t)
	);

	{
		std::lock_guard<std::mutex> lock(m_linearAllocatorPageManagerMutex);

		if (!m_linearAllocatorPageManagerMap.contains(pageManagerKey)) {
			m_linearAllocatorPageManagerMap.emplace(pageManagerKey, std::make_unique<LinearAllocatorPageManager>(
				m_pDevice.Get(),
				heapType,
				heapType == D3D12_HEAP_TYPE_UPLOAD ? 0x200000 : 0x10000
			));
		}
	}

	return *m_linearAllocatorPageManagerMap.at(pageManagerKey);
}

DescriptorHeapManager& Device::GetDescriptorHeapManager(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags)
{
	std::vector<uint8_t> descriptorHeapManagerKey(
		reinterpret_cast<const uint8_t*>(&descriptorHeapType),
		reinterpret_cast<const uint8_t*>(&descriptorHeapType) + sizeof(D3D12_DESCRIPTOR_HEAP_TYPE)
	);

	descriptorHeapManagerKey.insert(descriptorHeapManagerKey.end(),
		reinterpret_cast<const uint8_t*>(&descriptorHeapFlags),
		reinterpret_cast<const uint8_t*>(&descriptorHeapFlags) + sizeof(D3D12_DESCRIPTOR_HEAP_FLAGS)
	);

	{
		std::lock_guard<std::mutex> lock(m_descriptorHeapManagerMutex);

		if (!m_descriptorHeapManagerMap.contains(descriptorHeapManagerKey)) {
			m_descriptorHeapManagerMap.emplace(
				descriptorHeapManagerKey,
				std::make_unique<DescriptorHeapManager>(
					m_pDevice.Get(),
					descriptorHeapType,
					1024,
					descriptorHeapFlags
				)
			);
		}
	}

	return *m_descriptorHeapManagerMap.at(descriptorHeapManagerKey);
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> Device::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc)
{
	Microsoft::WRL::ComPtr<ID3DBlob> pRootSignatureBlob, pErrorBlob;
	CHECK_HRESULT(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pRootSignatureBlob, &pErrorBlob));

	std::vector<uint8_t> rootSignatureCacheKey(
		static_cast<uint8_t*>(pRootSignatureBlob->GetBufferPointer()),
		static_cast<uint8_t*>(pRootSignatureBlob->GetBufferPointer()) + pRootSignatureBlob->GetBufferSize()
	);

	{
		std::lock_guard<std::mutex> lock(m_rootSignatureCacheMutex);
		auto it = m_rootSignatureCache.find(rootSignatureCacheKey);
		if (it != m_rootSignatureCache.end()) {
			return it->second;
		}
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature;

	CHECK_HRESULT(m_pDevice->CreateRootSignature(
		0,
		pRootSignatureBlob->GetBufferPointer(),
		pRootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&pRootSignature)
	));

	{
		std::lock_guard<std::mutex> lock(m_rootSignatureCacheMutex);
		m_rootSignatureCache.emplace(rootSignatureCacheKey, pRootSignature);
	}

	return pRootSignature;
}