#pragma once

template<typename T>
inline T AlignUp(T size, size_t alignment) noexcept
{
	if (alignment > 0)
	{
		assert(((alignment - 1) & alignment) == 0);
		size_t mask = alignment - 1;
		return (T)(((size_t)size + mask) & ~mask);
	}
	return size;
}

template<typename T>
inline T AlignDown(T size, size_t alignment) noexcept
{
	if (alignment > 0)
	{
		assert(((alignment - 1) & alignment) == 0);
		size_t mask = alignment - 1;
		return (T)((size_t)size & ~mask);
	}
	return size;
}