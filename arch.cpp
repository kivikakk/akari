#include <arch.hpp>
#include <debug.hpp>
#include <Akari.hpp>
#include <Memory.hpp>

void __cxa_pure_virtual() {
	AkariPanic("__cxa_pure_virtual called");
}

void *operator new(size_t n) {
	ASSERT(Akari);
	ASSERT(Akari->memory);
	return Akari->memory->alloc(n);
}

void *operator new[](size_t n) {
	ASSERT(Akari);
	ASSERT(Akari->memory);
	return Akari->memory->alloc(n);
}

void *operator new(size_t, void *p) {
	return p;
}

void operator delete(void *p) {
	ASSERT(Akari);
	ASSERT(Akari->memory);
	return Akari->memory->free(p);
}

void operator delete[](void *p) {
	// These assertions seem quite wasteful.
	ASSERT(Akari);
	ASSERT(Akari->memory);
	return Akari->memory->free(p);
}
