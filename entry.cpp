#include <entry.hpp>
#include <debug.hpp>
#include <Akari.hpp>

void *AkariMultiboot;
void *AkariStack;

u32 placementAddress;

inline void *PlacementAlloc(u32 n, bool align=false) {
	if (align && (placementAddress & 0xFFF))
		placementAddress = (placementAddress & ~0xFFF) + 0x1000;
	
	u32 addr = placementAddress;
	placementAddress += n;
	return (void *)addr;
}

void AkariEntry() {
	// Bootstrap memory subsystems.
	u32 kernelStart = (u32)&__kstart,
		kernelEnd = (u32)&__kend;

	// We'll start allocating from here
	placementAddress = kernelEnd;

	Kernel = new (PlacementAlloc(sizeof(Akari))) Akari();
	Kernel->Console = new (PlacementAlloc(sizeof(AkariConsoleSubsystem))) AkariConsoleSubsystem(Kernel);

	Kernel->Console->PutString("Are we here yet?");
}

