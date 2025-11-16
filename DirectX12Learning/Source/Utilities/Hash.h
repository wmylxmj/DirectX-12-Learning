#pragma once

#include <intrin.h>
#include <vector>

#include "../Math/Align.h"

inline bool IsSSE42Supported()
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

		size_t hashValue = 2166136261U;

		if (isSSE42Supported)
		{
			const uint64_t* iter64 = (const uint64_t*)AlignUp(data.data(), 8);
			const uint64_t* end64 = (const uint64_t*)AlignDown(data.data() + data.size(), 8);

			if ((uint8_t*)iter64 > data.data()) {
				for (const uint8_t* bytePtr = data.data(); bytePtr < (uint8_t*)iter64; ++bytePtr) {
					hashValue = _mm_crc32_u8((uint32_t)hashValue, *bytePtr);
				}
			}

			while (iter64 < end64)
			{
				hashValue = _mm_crc32_u64((uint64_t)hashValue, *iter64);
				++iter64;
			}

			if ((uint8_t*)end64 < data.data() + data.size()) {
				for (const uint8_t* bytePtr = (uint8_t*)end64; bytePtr < data.data() + data.size(); ++bytePtr) {
					hashValue = _mm_crc32_u8((uint32_t)hashValue, *bytePtr);
				}
			}
		}
		else
		{
			for (const uint8_t* bytePtr = data.data(); bytePtr < data.data() + data.size(); ++bytePtr) {
				hashValue = 16777619U * hashValue ^ *bytePtr;
			}
		}

		return hashValue;
	}
};