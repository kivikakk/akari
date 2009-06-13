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

void *operator new(u32 n) {
	ASSERT(Akari);
	ASSERT(Akari->Memory);
	return Akari->Memory->Alloc(n);
}

void *operator new[](u32 n) {
	ASSERT(Akari);
	ASSERT(Akari->Memory);
	return Akari->Memory->Alloc(n);
}

void *operator new(u32, void *p) {
	return p;
}

void operator delete(void *p) {
	ASSERT(Akari);
	ASSERT(Akari->Memory);
	return Akari->Memory->Free(p);
}

void operator delete[](void *p) {
	// These assertions seem quite wasteful.
	ASSERT(Akari);
	ASSERT(Akari->Memory);
	return Akari->Memory->Free(p);
}
