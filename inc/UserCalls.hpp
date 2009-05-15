#ifndef __USER_CALLS_HPP__
#define __USER_CALLS_HPP__

#include <arch.hpp>

namespace User {
	void Puts(const char *s);
	void Putl(u32 n, u8 base);
	u32 GetProcessId();
	void IrqWait(u32 irq);
}

#endif

