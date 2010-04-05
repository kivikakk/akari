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

#include <BlockingCall.hpp>
#include <Timer.hpp>
#include <counted_ptr>

namespace User {
	void putc(char c);
	void puts(const char *s);
	u32 getProcessId();
	void irqWait();
	bool irqWaitTimeout(u32 ms);
	void irqListen(u32 irq);
	u32 ticks();
	void panic(const char *s);
	void sysexit();
	void defer();
	void *malloc(u32 n);
	void *mallocap(u32 n, void **phys);
	void free(void *p);
	void flushTLB();

	class IRQWaitCall : public BlockingCall {
	public:
		IRQWaitCall(u32 timeout);
		~IRQWaitCall();

		u32 operator ()();

		static Symbol type();
		Symbol insttype() const;

	protected:
		u32 timeout;
		counted_ptr<TimerEventWakeup> event;
	};
}

#elif defined(__AKARI_LINKAGE)

DEFN_SYSCALL1(putc, 0, void, char);
DEFN_SYSCALL1(puts, 1, void, const char *);
DEFN_SYSCALL0(getProcessId, 3, u32);
DEFN_SYSCALL0(irqWait, 4, void);
DEFN_SYSCALL1(irqWaitTimeout, 38, bool, u32);
DEFN_SYSCALL1(irqListen, 5, void, u32);
DEFN_SYSCALL0(ticks, 37, u32);
DEFN_SYSCALL1(panic, 6, void, const char *);
DEFN_SYSCALL0(sysexit, 7, void);
DEFN_SYSCALL0(defer, 8, void);
DEFN_SYSCALL1(malloc, 9, void *, u32);
DEFN_SYSCALL2(mallocap, 40, void *, u32, void **);
DEFN_SYSCALL1(free, 10, void, void *);
DEFN_SYSCALL0(flushTLB, 41, void);

#else

DECL_SYSCALL1(putc, void, char);
DECL_SYSCALL1(puts, void, const char *);
DECL_SYSCALL0(getProcessId, u32);
DECL_SYSCALL0(irqWait, void);
DECL_SYSCALL1(irqWaitTimeout, bool, u32);
DECL_SYSCALL1(irqListen, void, u32);
DECL_SYSCALL0(ticks, u32);
DECL_SYSCALL1(panic, void, const char *);
DECL_SYSCALL0(sysexit, void);
DECL_SYSCALL0(defer, void);
DECL_SYSCALL1(malloc, void *, u32);
DECL_SYSCALL2(mallocap, void *, u32, void **);
DECL_SYSCALL1(free, void, void *);
DECL_SYSCALL0(flushTLB, void);

#endif

#endif

