#include <Akari.hpp>
#include <debug.hpp>

Kernel *Akari;

Kernel::Kernel(): memory(0), console(0), descriptor(0), timer(0), tasks(0), syscall(0) {
}

/**
 * Constructs an Kernel, using the given address as the current placement address.
 */
Kernel *Kernel::Construct(u32 addr, u32 upperMemory) {
	Kernel *kernel = new ((void *)addr) Kernel();
	addr += sizeof(Kernel);
	kernel->memory = new ((void *)addr) Memory(upperMemory);
	addr += sizeof(Memory);

	kernel->memory->setPlacementMode(addr);

	return kernel;
}
