#include <Akari.hpp>
#include <debug.hpp>

AkariKernel *Akari;

AkariKernel::AkariKernel(): Console(0), Memory(0) {
}

void AkariKernel::Assert(bool condition) {
	if (!condition)
		AkariPanic("Assertion failed somewhere!");
}
