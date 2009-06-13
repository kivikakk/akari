#include <arch.hpp>
#include <debug.hpp>
#include <Akari.hpp>

void AkariOutB(u16 port, u8 data) {
	asm volatile("outb %1, %0" : : "dN" (port), "a" (data));
}

void AkariOutW(u16 port, u16 data) {
	asm volatile("outw %1, %0" : : "dN" (port), "a" (data));
}

u8 AkariInB(u16 port) {
	u8 r;
	asm volatile("inb %1, %0" : "=a" (r) : "dN" (port));
	return r;
}

u16 AkariInW(u16 port) {
	u16 r;
	asm volatile("inw %1, %0" : "=a" (r) : "dN" (port));
	return r;
}

void __cxa_pure_virtual() {
	AkariPanic("__cxa_pure_virtual called");
}

void *operator new(size_t n) {
	ASSERT(Akari);
	ASSERT(Akari->Memory);
	return Akari->Memory->alloc(n);
}

void *operator new[](size_t n) {
	ASSERT(Akari);
	ASSERT(Akari->Memory);
	return Akari->Memory->alloc(n);
}

void *operator new(size_t, void *p) {
	return p;
}

void operator delete(void *p) {
	ASSERT(Akari);
	ASSERT(Akari->Memory);
	return Akari->Memory->free(p);
}

void operator delete[](void *p) {
	// These assertions seem quite wasteful.
	ASSERT(Akari);
	ASSERT(Akari->Memory);
	return Akari->Memory->free(p);
}
