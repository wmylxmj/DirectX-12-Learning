#include "Resource.h"

ID3D12Resource* Resource::GetResource() const
{
	return m_pResource.Get();
}