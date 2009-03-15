#include <arch.hpp>

void __cxa_pure_virtual() {
	// TODO: panic or something!
}

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
