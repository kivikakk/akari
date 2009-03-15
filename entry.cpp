#include <entry.hpp>
#include <debug.hpp>
#include <Akari.hpp>

void *AkariMultiboot;
void *AkariStack;

void AkariEntry() {
	// Bootstrap memory subsystems.
	u32 kernelStart = (u32)&__kstart,
		kernelEnd = (u32)&__kend;

	// We'll start allocating from here
	u32 placementAddress = kernelEnd;

	Akari = new ((void *)placementAddress) AkariKernel();
	placementAddress += sizeof(AkariKernel);
	Akari->Memory = new ((void *)placementAddress) AkariMemorySubsystem();
	placementAddress += sizeof(AkariMemorySubsystem);

	Akari->Memory->SetPlacementMode(placementAddress);
	// we can now use `new' to use the memory manager's placement

	Akari->Console = new AkariConsoleSubsystem();
	Akari->Console->PutString("Testing!");
}

