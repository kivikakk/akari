#include <UserCalls.hpp>
#include <Akari.hpp>

namespace User {
	void Putc(char c) {
		Akari->Console->PutChar(c);
	}

	void Puts(const char *s) {
		Akari->Console->PutString(s);
	}

	void Putl(u32 n, u8 base) {
		Akari->Console->PutInt(n, base);
	}
	u32 GetProcessId() {
		return Akari->Task->current->_id;
	}

	void IrqWait() {
		if (Akari->Task->current->irqListenHits == 0) {
			Akari->Task->current->irqWaiting = true;

			Akari->Syscall->ReturnToNextTask();
			return;
		}

		Akari->Task->current->irqListenHits--;
	}

	void IrqListen(u32 irq) {
		Akari->Task->current->irqListen = irq;
		Akari->Task->current->irqListenHits = 0;
	}


	void Panic(const char *s) {
		// TODO: check permission. should be ring 1 or 0, but at least definitely not 3.
		// even ring 1 tasks should be able to die and come back up. that's the point, right?
		// so ring 1 tasks panicking should, like ring 3 ones, just be killed off. (and
		// the idea is that the system notes this and fixes it. yep) (TODO!)
		AkariPanic(s);
	}
}

