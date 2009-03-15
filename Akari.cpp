#include <Akari.hpp>
#include <debug.hpp>

AkariKernel *Akari;

AkariKernel::AkariKernel(): Memory(0), Console(0), Descriptor(0) {
}

void AkariKernel::Assert(bool condition) {
	if (!condition)
		AkariPanic("Assertion failed somewhere!");
}
