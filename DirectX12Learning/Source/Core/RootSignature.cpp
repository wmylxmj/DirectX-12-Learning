#include "RootSignature.h"

std::mutex RootSignature::sm_rootSignatureCacheMutex;
std::map<std::vector<uint8_t>, Microsoft::WRL::ComPtr<ID3D12RootSignature>> RootSignature::sm_rootSignatureCache;

RootSignature::RootSignature(UINT numParameters, UINT numStaticSamplers)
{
	Reset(numParameters, numStaticSamplers);
}

void RootSignature::Reset(UINT numParameters, UINT numStaticSamplers)
{
	if (numParameters > 0) {
		m_parameters.reset(new RootParameter[numParameters]);
	}
	else {
		m_parameters = nullptr;
	}
	m_numParameters = numParameters;

	if (numStaticSamplers > 0) {
		m_staticSamplers.reset(new CD3DX12_STATIC_SAMPLER_DESC[numStaticSamplers]);
	}
	else {
		m_staticSamplers = nullptr;
	}
	m_numStaticSamplers = numStaticSamplers;
}

RootParameter& RootSignature::operator[](size_t index)
{
	assert(index < m_numParameters);
	return m_parameters.get()[index];
}

const RootParameter& RootSignature::operator[](size_t index) const
{
	assert(index < m_numParameters);
	return m_parameters.get()[index];
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature::GetRootSignature() const
{
	return m_rootSignature;
}

void RootSignature::CreateRootSignature(ID3D12Device* pDevice, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.NumParameters = m_numParameters;
	rootSignatureDesc.pParameters = (const D3D12_ROOT_PARAMETER*)m_parameters.get();
	rootSignatureDesc.NumStaticSamplers = m_numStaticSamplers;
	rootSignatureDesc.pStaticSamplers = (const D3D12_STATIC_SAMPLER_DESC*)m_staticSamplers.get();
	rootSignatureDesc.Flags = flags;

	m_cbvSrvUavDescriptorTableBitMap = 0;
	m_samplerDescriptorTableBitMap = 0;

	for (UINT i = 0; i < m_numParameters; ++i) {
		const D3D12_ROOT_PARAMETER& parameter = rootSignatureDesc.pParameters[i];
		m_descriptorTableSize[i] = 0;

		if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
			assert(parameter.DescriptorTable.pDescriptorRanges != nullptr);

			// ±ê¼ÇÎ»Í¼
			if (parameter.DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
				m_samplerDescriptorTableBitMap |= ((uint64_t)1 << i);
			}
			else
			{
				m_cbvSrvUavDescriptorTableBitMap |= ((uint64_t)1 << i);
			}

			for (UINT j = 0; j < parameter.DescriptorTable.NumDescriptorRanges; ++j) {
				m_descriptorTableSize[i] += parameter.DescriptorTable.pDescriptorRanges[j].NumDescriptors;
			}
		}

		Microsoft::WRL::ComPtr<ID3DBlob> pRootSignatureBlob, pErrorBlob;

		CHECK_HRESULT(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pRootSignatureBlob, &pErrorBlob));

		std::vector<uint8_t> blobKey(
			static_cast<uint8_t*>(pRootSignatureBlob->GetBufferPointer()),
			static_cast<uint8_t*>(pRootSignatureBlob->GetBufferPointer()) + pRootSignatureBlob->GetBufferSize()
		);

		{
			std::lock_guard<std::mutex> lock(sm_rootSignatureCacheMutex);
			auto it = sm_rootSignatureCache.find(blobKey);
			if (it != sm_rootSignatureCache.end()) {
				m_rootSignature = it->second;
				return;
			}
		}

		CHECK_HRESULT(pDevice->CreateRootSignature(
			0,
			pRootSignatureBlob->GetBufferPointer(),
			pRootSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSignature)
		));
		{
			std::lock_guard<std::mutex> lock(sm_rootSignatureCacheMutex);
			sm_rootSignatureCache[blobKey] = m_rootSignature;
		}
	}
}