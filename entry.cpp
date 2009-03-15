#include <entry.hpp>
#include <debug.hpp>
#include <Akari.hpp>

u32 AkariKernelStart, AkariKernelEnd;

void *AkariMultiboot;
void *AkariStack;

void AkariEntry() {
	// Bootstrap memory subsystems.
	AkariKernelStart = (u32)&__kstart;
	AkariKernelEnd = (u32)&__kend;
}

