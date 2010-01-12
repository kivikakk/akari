#ifndef __USER_CALLS_HPP__
#define __USER_CALLS_HPP__

#include <arch.hpp>
#include <UserGates.hpp>

#ifdef __AKARI_KERNEL__

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

#else

DECL_SYSCALL1(putc, char);
DECL_SYSCALL1(puts, const char *);
DECL_SYSCALL2(putl, u32, u8);
DECL_SYSCALL0(getProcessId);
DECL_SYSCALL0(irqWait);
DECL_SYSCALL1(irqListen, u32);
DECL_SYSCALL1(panic, const char *);
DECL_SYSCALL0(exit);
DECL_SYSCALL0(defer);
DECL_SYSCALL1(malloc, u32);
DECL_SYSCALL1(free, void *);
DECL_SYSCALL3(memcpy, void *, const void *, u32);

#endif

#endif

