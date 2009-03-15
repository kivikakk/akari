#include <Akari.hpp>
#include <debug.hpp>

Akari *Kernel;

Akari::Akari(): Console(0), Memory(0) {
}

void Akari::Assert(bool condition) {
	if (!condition)
		AkariPanic("Assertion failed somewhere!");
}
