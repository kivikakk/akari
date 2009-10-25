#ifndef __ARCH_HPP__
#define __ARCH_HPP__

// These are specific to my architecture, and will need to be expanded
// to a proper system later.

typedef __SIZE_TYPE__ size_t;
typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef signed long s32;
typedef signed short s16;
typedef signed char s8;

inline void AkariOutB(u16 port, u8 data) {
	asm volatile("outb %1, %0" : : "dN" (port), "a" (data));
}

inline void AkariOutW(u16 port, u16 data) {
	asm volatile("outw %1, %0" : : "dN" (port), "a" (data));
}

inline u8 AkariInB(u16 port) {
	u8 r;
	asm volatile("inb %1, %0" : "=a" (r) : "dN" (port));
	return r;
}

inline u16 AkariInW(u16 port) {
	u16 r;
	asm volatile("inw %1, %0" : "=a" (r) : "dN" (port));
	return r;
}

// Compiler-specific things.
extern "C" void __cxa_pure_virtual();
void *operator new(size_t);
void *operator new[](size_t);
void *operator new(size_t, void *);
void operator delete(void *);
void operator delete[](void *);

#endif

