#ifndef __USER_CALLS_HPP__
#define __USER_CALLS_HPP__

#include <arch.hpp>

namespace User {
	void putc(char c);
	void puts(const char *s);
	void putl(u32 n, u8 base);
	u32 getProcessId();
	void irqWait();
	void irqListen(u32 irq);
	void panic(const char *s);
	bool registerName(const char *name);
	bool registerNode(const char *name);
	void exit();
	u32 obtainNodeWriter(const char *name, const char *node, bool exclusive);
	u32 obtainNodeListener(const char *name, const char *node);
	u32 readNode(const char *name, const char *node, u32 listener, char *buffer, u32 n);
	u32 readNodeUnblock(const char *name, const char *node, u32 listener, char *buffer, u32 n);
	u32 writeNode(const char *name, const char *node, u32 writer, const char *buffer, u32 n);
	void defer();
}

#endif

