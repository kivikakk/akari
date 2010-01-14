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

#ifndef __USER_CALLS_HPP__
#define __USER_CALLS_HPP__

#include <arch.hpp>
#include <UserGates.hpp>

#if defined(__AKARI_KERNEL)

namespace User {
	void putc(char c);
	void puts(const char *s);
	void putl(u32 n, u8 base);
	u32 getProcessId();
	void irqWait();
	void irqListen(u32 irq);
	void panic(const char *s);
	void exit();
	void defer();
	void *malloc(u32 n);
	void free(void *p);
	void *memcpy(void *dest, const void *src, u32 n);
}

#elif defined(__AKARI_LINKAGE)

DEFN_SYSCALL1(putc, 0, void, char);
DEFN_SYSCALL1(puts, 1, void, const char *);
DEFN_SYSCALL2(putl, 2, void, u32, u8);
DEFN_SYSCALL0(getProcessId, 3, u32);
DEFN_SYSCALL0(irqWait, 4, void);
DEFN_SYSCALL1(irqListen, 5, void, u32);
DEFN_SYSCALL1(panic, 6, void, const char *);
DEFN_SYSCALL0(exit, 7, void);
DEFN_SYSCALL0(defer, 8, void);
DEFN_SYSCALL1(malloc, 9, void *, u32);
DEFN_SYSCALL1(free, 10, void, void *);
DEFN_SYSCALL3(memcpy, 11, void *, void *, const void *, u32);

#else

DECL_SYSCALL1(putc, void, char);
DECL_SYSCALL1(puts, void, const char *);
DECL_SYSCALL2(putl, void, u32, u8);
DECL_SYSCALL0(getProcessId, u32);
DECL_SYSCALL0(irqWait, void);
DECL_SYSCALL1(irqListen, void, u32);
DECL_SYSCALL1(panic, void, const char *);
DECL_SYSCALL0(exit, void);
DECL_SYSCALL0(defer, void);
DECL_SYSCALL1(malloc, void *, u32);
DECL_SYSCALL1(free, void, void *);
DECL_SYSCALL3(memcpy, void *, void *, const void *, u32);

#endif

#endif

