#include "Resource.h"

ID3D12Resource* Resource::GetResource()
{
	return m_pResource.Get();
}