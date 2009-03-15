#ifndef __ENTRY_HPP__
#define __ENTRY_HPP__

#include <arch.hpp>

extern "C" {
	extern u8 __kstart, __kend;
	extern u32 AkariKernelStart, AkariKernelEnd;

	extern void *AkariMultiboot;
	extern void *AkariStack;

	void AkariEntry();
}

#endif

