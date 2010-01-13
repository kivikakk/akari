// This file is part of Akari.
// Copyright 2010 Arlen Cuss
//
// Akari is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Akari is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Akari.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __ARCH_HPP__
#define __ARCH_HPP__

// These are specific to my architecture, and will need to be expanded
// to a proper system later.

typedef __SIZE_TYPE__ size_t;
typedef unsigned long u32, pid_t;
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

