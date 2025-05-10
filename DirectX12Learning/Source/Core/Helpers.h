#pragma once

#include "PrecompiledHeader.h"

template<typename T>
inline T AlignUp(T size, size_t alignment) noexcept
{
	if (alignment > 0)
	{
		assert(((alignment - 1) & alignment) == 0);
		auto mask = static_cast<T>(alignment - 1);
		return (size + mask) & ~mask;
	}
	return size;
}
