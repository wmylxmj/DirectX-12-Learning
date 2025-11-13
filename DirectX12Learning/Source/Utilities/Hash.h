#pragma once

#include <intrin.h>

bool IsSSE42Supported()
{
	int CPUInfo[4];
	__cpuid(CPUInfo, 1);
	return (CPUInfo[2] & 0x100000) != 0;
}