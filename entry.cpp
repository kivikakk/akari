#include <entry.hpp>
#include <debug.hpp>
#include <Akari.hpp>

void *AkariMultiboot;
void *AkariStack;

void AkariEntry() {
	Kernel = 0;
	Kernel->Console->PutString("Hello, world!\n");
}

