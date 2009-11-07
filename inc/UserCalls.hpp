#ifndef __USER_CALLS_HPP__
#define __USER_CALLS_HPP__

#include <arch.hpp>
#include <Tasks.hpp>

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

#endif

