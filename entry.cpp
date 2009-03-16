#include <entry.hpp>
#include <debug.hpp>
#include <Akari.hpp>

void *AkariMultiboot;
void *AkariStack;

void AkariEntry() {
	// Bootstrap memory subsystems; we'll start allocating from here.
	u32 placementAddress = (u32)&__kend;

	Akari = new ((void *)placementAddress) AkariKernel();
	placementAddress += sizeof(AkariKernel);
	Akari->Memory = new ((void *)placementAddress) AkariMemorySubsystem();
	placementAddress += sizeof(AkariMemorySubsystem);

	Akari->Memory->SetPlacementMode(placementAddress);
	// we can now use `new' to use the memory manager's placement

	Akari->Console = new AkariConsoleSubsystem();
	Akari->Descriptor = new AkariDescriptorSubsystem();

	Akari->Console->PutString("\nSystem halted!");
	while (1)
		asm volatile("hlt");
}

