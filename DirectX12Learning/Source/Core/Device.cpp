#include "Device.h"

Device::Device(IUnknown* pAdapter)
{
	CHECK_HRESULT(D3D12CreateDevice(
		pAdapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_pDevice)
	));
}