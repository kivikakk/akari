#ifndef __ARCH_HPP__
#define __ARCH_HPP__

// These are specific to my architecture, and will need to be expanded
// to a proper system later.

typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef signed long s32;
typedef signed short s16;
typedef signed char s8;

// Minor IO functions. May need to be moved elsewhere later? (arch-dep)

extern "C" void AkariOutB(u16, u8);
extern "C" void AkariOutW(u16, u16);
extern "C" u8 AkariInB(u16);
extern "C" u16 AkariInW(u16);

// C++ pure virtual function called?!

extern "C" void __cxa_pure_virtual();

#endif

