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

		// irq0();	// do what the timer does. seems a fairly cheap (as in hackish) way to do things, though. スタックとか大丈夫かな
		// the above blatently doesn't work. hm.

		// this below code DOES, but isn't it the dirtiest hack ever?! also slow, since we don't return immediately.
		__asm__ __volatile__("sti; hlt");

		Akari->Console->PutString("[>] ");
		Akari->Task->current->irqWait = 0;
	}
}

