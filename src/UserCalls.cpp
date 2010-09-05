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

#include <UserCalls.hpp>

#if defined(__AKARI_KERNEL)
#include <Akari.hpp>
#include <Symbol.hpp>
#include <debug.hpp>
#include <Console.hpp>
#include <Tasks.hpp>
#include <Syscall.hpp>
#include <BlockingCall.hpp>
#include <Timer.hpp>
#include <Debugger.hpp>
#include <UserIPC.hpp>
#include <UserPrivs.hpp>

namespace User {
	void putc(char c) {
		requirePrivilege(PRIV_CONSOLE_WRITE);
		Akari->console->putChar(c);
	}

	void puts(const char *s) {
		requirePrivilege(PRIV_CONSOLE_WRITE);
		Akari->console->putString(s);
	}

	u32 getProcessId() {
		return Akari->tasks->current->id;
	}

	IRQWaitCall::IRQWaitCall(u32 timeout): timeout(timeout), event(0)
	{ }

	IRQWaitCall::~IRQWaitCall() {
		if (event) {
			event = counted_ptr<TimerEventWakeup>(0);
		}
	}

	u32 IRQWaitCall::operator ()() {
		if (Akari->tasks->current->irqListenHits == 0) {
			if (timeout &&
					AkariMicrokernelSwitches >=
					(Akari->tasks->current->irqWaitStart + timeout / 10)) {
				// XXX magic number: 1000ms per second, 100 ticks per second
				// -> timeout ms/10 gives no. of ticks for timeout
				// Should already be descheduled.
				event = counted_ptr<TimerEventWakeup>(0);

				_wontBlock();
				return static_cast<u32>(false);
			} else {
				if (timeout) {
					event = counted_ptr<TimerEventWakeup>(
							new TimerEventWakeup(
								Akari->tasks->current->irqWaitStart + timeout / 10,
								Akari->tasks->current));
					Akari->timer->at(event);
				}

				_willBlock();
				return 0;
			}
		}

		if (event) {
			Akari->timer->desched(*event);
			event = counted_ptr<TimerEventWakeup>(0);
		}

		_wontBlock();
		Akari->tasks->current->irqListenHits--;
		return static_cast<u32>(true);
	}

	Symbol IRQWaitCall::type() { return Symbol("IRQWaitCall"); }
	Symbol IRQWaitCall::insttype() const { return type(); }

	void irqWait() {
		requirePrivilege(PRIV_IRQ);

		Akari->tasks->current->irqWaitStart = 0;

		IRQWaitCall c(0);
		c();
		if (!c.shallBlock())
			return;

		Akari->tasks->current->userWaiting = true;
		Akari->tasks->current->userCall = new IRQWaitCall(c);
		Akari->syscall->returnToNextTask();
		return;
	}

	bool irqWaitTimeout(u32 ms) {
		requirePrivilege(PRIV_IRQ);

		Akari->tasks->current->irqWaitStart = AkariMicrokernelSwitches;

		IRQWaitCall c(ms);
		u32 r = c();
		if (!c.shallBlock())
			return static_cast<bool>(r);

		Akari->tasks->current->userWaiting = true;
		Akari->tasks->current->userCall = new IRQWaitCall(c);
		Akari->syscall->returnToNextTask();
		return false;
	}

	void irqListen(u32 irq) {
		requirePrivilege(PRIV_IRQ);

		Akari->tasks->current->irqListen = irq;
		Akari->tasks->current->irqListenHits = 0;
	}

	u32 ticks() {
		return AkariMicrokernelSwitches;
	}

	void panic(const char *s) {
		Akari->console->printf("Process 0x%x \"%s\" ", Akari->tasks->current->id, Akari->tasks->current->name.c_str());
		const char *rn = Akari->tasks->current->registeredName.c_str();
		if (rn) {
			Akari->console->printf("(:%s) ", rn);
		}
		Akari->console->printf("dying (panic'd \"%s\")\n", s);

		struct modeswitch_registers *r = reinterpret_cast<modeswitch_registers *>(Akari->tasks->current->ks);
		Akari->console->printf("EIP was %x, ESP was %x, EBP was %x, EFLAGS was %x, USERESP was %x\n",
			r->callback.eip, r->callback.esp, r->callback.ebp, r->callback.eflags, r->useresp);

		u8 *ra = reinterpret_cast<u8 *>(r);
		for (int i = 0; i < 128; ++i) {
			Akari->console->printf("%s%x ", *ra >= 0x10 ? "" : "0", *ra);
			if (i % 16 == 15) Akari->console->printf("\n");
			if (i % 16 == 7) Akari->console->printf(" ");
			++ra;
		}

		u32 coresize;
		u8 *core = Akari->tasks->current->dumpELFCore(&coresize);
		Akari->debugger->setReceiveFile(core, coresize);
		delete [] core;

		// when exit becomes more complicated later we may have to do cleanup
		// instead of just a sysexit, may not be ideal for a process that's
		// dieing because it sucks.
		//Akari->debugger->run();
		sysexit();
	}

	void sysexit() {
		for (std::map<Symbol, Tasks::Task *>::iterator it = Akari->tasks->registeredTasks.begin();
				it != Akari->tasks->registeredTasks.end(); ++it) {
			if (it->second == Akari->tasks->current) {
				Akari->tasks->registeredTasks.erase(it->first);
				break;
			}
		}

		Tasks::Task *task = Akari->tasks->start;
		while (task) {
			if (task == Akari->tasks->current) {
				task = task->next;
				continue;
			}

			task->unblockTypeWith(IPC::WaitProcessCall::type(), Akari->tasks->current->id);
			task = task->next;
		}

		Akari->syscall->returnToNextTask();
		// Find the Task* which refers to Akari->tasks->current, and get it to skip it.
		Tasks::Task **scanner = &Akari->tasks->start;
		while (*scanner != Akari->tasks->current) {
			scanner = &(*scanner)->next;
		}

		*scanner = (*scanner)->next;
		// Gone! XXX what happens when the last task exists!? Everything probably goes to hell ...
		// This should never happen because of the idle task; right?
	}

	void defer() {
		Akari->syscall->returnToNextTask();
	}

	void *malloc(u32 n) {
		requirePrivilege(PRIV_MALLOC);

		ASSERT(Akari->tasks->current->heap);
		return Akari->tasks->current->heap->alloc(n);
	}

	void *mallocap(u32 n, phptr *p) {
		requirePrivilege(PRIV_MALLOC);
		requirePrivilege(PRIV_PHYSADDR);

		void *mem = Akari->tasks->current->heap->allocAligned(n);
		*p = physAddr(mem);
		return mem;
	}

	phptr physAddr(void *ptr) {
		requirePrivilege(PRIV_PHYSADDR);

		Memory::Page *page = Akari->tasks->current->pageDir->getPage(reinterpret_cast<u32>(ptr), false);
		if (!page) return 0;
		return page->pageAddress * 0x1000 + (reinterpret_cast<u32>(ptr) & 0xFFF);
	}

	void free(void *p) {
		requirePrivilege(PRIV_MALLOC);

		ASSERT(Akari->tasks->current->heap);
		bool success = Akari->tasks->current->heap->free(p);
		if (!success) {
			panic("free returned false");
		}
	}
}
#endif
