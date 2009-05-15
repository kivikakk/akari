#include <UserCalls.hpp>
#include <Akari.hpp>

namespace User {
	void Puts(const char *s) {
		Akari->Console->PutString(s);
	}

	void Putl(u32 n, u8 base) {
		Akari->Console->PutInt(n, base);
	}
	u32 GetProcessId() {
		return Akari->Task->current->_id;
	}

	void IrqWait(u32 irq) {
		Akari->Task->current->irqWait = irq;
		Akari->Console->PutString("[<] ");

		// causes the timer event to fire now. could this screw up our tick counting? hm. :-(
		__asm__ __volatile__("int $0x20");
		// tick counting is dumb anyway.

		Akari->Console->PutString("[>] ");
		Akari->Task->current->irqWait = 0;
	}
}

