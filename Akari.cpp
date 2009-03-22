#include <Akari.hpp>
#include <debug.hpp>

AkariKernel *Akari;

AkariKernel::AkariKernel(): Memory(0), Console(0), Descriptor(0), Timer(0) {
}

/**
 * Constructs an AkariKernel, using the given address as the current placement address.
 */
AkariKernel *AkariKernel::Construct(u32 addr, u32 upperMemory) {
	AkariKernel *kernel = new ((void *)addr) AkariKernel();
	addr += sizeof(AkariKernel);
	kernel->Memory = new ((void *)addr) AkariMemorySubsystem(upperMemory);
	addr += sizeof(AkariMemorySubsystem);

	kernel->Memory->SetPlacementMode(addr);

	return kernel;
}
