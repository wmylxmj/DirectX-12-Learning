#pragma once

#include <intrin.h>
#include <vector>

bool IsSSE42Supported()
{
	int CPUInfo[4];
	__cpuid(CPUInfo, 1);
	return (CPUInfo[2] & 0x100000) != 0;
}

template<typename T>
struct Hash
{
	size_t operator()(const T& data) const
	{
		return std::hash<T>()(data);
	}
};

template<>
struct Hash<std::vector<uint8_t>>
{
	size_t operator()(const std::vector<uint8_t>& data) const
	{
		static bool isSSE42Supported = IsSSE42Supported();

		if (IsSSE42Supported())
		{
			uint32_t crc = 0xFFFFFFFF;
			size_t length = data.size();
			const uint8_t* ptr = data.data();
			size_t i = 0;
			// Process 8 bytes at a time
			for (; i + 8 <= length; i += 8)
			{
				crc = _mm_crc32_u64(crc, *reinterpret_cast<const uint64_t*>(ptr + i));
			}
			// Process remaining bytes
			for (; i < length; ++i)
			{
				crc = _mm_crc32_u8(crc, ptr[i]);
			}
			return static_cast<size_t>(crc);
		}
		else
		{
			// Fallback hash function (e.g., std::hash)
			size_t hash = 0;
			for (const auto& byte : data)
			{
				hash ^= std::hash<uint8_t>()(byte) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			}
			return hash;
		}
	}
};