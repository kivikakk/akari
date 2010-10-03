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
		mu_console->putChar(c);
	}

	void puts(const char *s) {
		requirePrivilege(PRIV_CONSOLE_WRITE);
		mu_console->putString(s);
	}

	u32 getProcessId() {
		return mu_tasks->current->id;
	}

	IRQWaitCall::IRQWaitCall(u32 timeout): timeout(timeout), wakeeventId(0)
	{ }

	u32 IRQWaitCall::operator ()() {
		if (mu_tasks->current->irqListenHits == 0) {
			if (timeout && !wakeeventId) {
				wakeeventId = mu_timer->wakeIn(timeout * 1000, mu_tasks->current);
				_willBlock();
				return 0;
			} else if (timeout) {
				// we've timed out!
				wakeeventId = 0;
				_wontBlock();
				return static_cast<u32>(false);
			} else {
				// no timeout.
				_willBlock();
				return 0;
			}
		}

		if (wakeeventId) {
			ASSERT(mu_timer->desched(wakeeventId));
			wakeeventId = 0;
		}

		_wontBlock();
		mu_tasks->current->irqListenHits--;
		return static_cast<u32>(true);
	}

	Symbol IRQWaitCall::type() { return Symbol("IRQWaitCall"); }
	Symbol IRQWaitCall::insttype() const { return type(); }

	void irqWait() {
		requirePrivilege(PRIV_IRQ);

		IRQWaitCall c(0);
		c();
		if (!c.shallBlock())
			return;

		mu_tasks->current->userWaiting = true;
		mu_tasks->current->userCall = new IRQWaitCall(c);
		mu_syscall->returnToNextTask();
		return;
	}

	bool irqWaitTimeout(u32 ms) {
		requirePrivilege(PRIV_IRQ);

		IRQWaitCall c(ms);
		u32 r = c();
		if (!c.shallBlock())
			return static_cast<bool>(r);

		mu_tasks->current->userWaiting = true;
		mu_tasks->current->userCall = new IRQWaitCall(c);
		mu_syscall->returnToNextTask();
		return false;
	}

	void irqListen(u32 irq) {
		requirePrivilege(PRIV_IRQ);

		mu_tasks->current->irqListen = irq;
		mu_tasks->current->irqListenHits = 0;
	}

	bool irqCheck() {
		if (!mu_tasks->current->irqListenHits)
			return false;
		mu_tasks->current->irqListenHits--;
		return true;
	}

	u32 ticks() {
		return AkariMicrokernelSwitches;
	}

	u32 tickHz() {
		return AkariTickHz;
	}

	void panic(const char *s) {
		mu_console->printf("Process 0x%x \"%s\" ", mu_tasks->current->id, mu_tasks->current->name.c_str());
		const char *rn = mu_tasks->current->registeredName.c_str();
		if (rn) {
			mu_console->printf("(:%s) ", rn);
		}
		mu_console->printf("dying (panic'd \"%s\")\n", s);

		struct modeswitch_registers *r = reinterpret_cast<modeswitch_registers *>(mu_tasks->current->ks);
		mu_console->printf("EIP was %x, ESP was %x, EBP was %x, EFLAGS was %x, USERESP was %x\n",
			r->callback.eip, r->callback.esp, r->callback.ebp, r->callback.eflags, r->useresp);

		u8 *ra = reinterpret_cast<u8 *>(r);
		for (int i = 0; i < 128; ++i) {
			mu_console->printf("%s%x ", *ra >= 0x10 ? "" : "0", *ra);
			if (i % 16 == 15) mu_console->printf("\n");
			if (i % 16 == 7) mu_console->printf(" ");
			++ra;
		}

		u32 coresize;
		u8 *core = mu_tasks->current->dumpELFCore(&coresize);
		mu_debugger->setReceiveFile(core, coresize);
		delete [] core;

		// when exit becomes more complicated later we may have to do cleanup
		// instead of just a sysexit, may not be ideal for a process that's
		// dieing because it sucks.
		//mu_debugger->run();
		sysexit();
	}

	void sysexit() {
		for (std::map<Symbol, Tasks::Task *>::iterator it = mu_tasks->registeredTasks.begin();
				it != mu_tasks->registeredTasks.end(); ++it) {
			if (it->second == mu_tasks->current) {
				mu_tasks->registeredTasks.erase(it->first);
				break;
			}
		}

		Tasks::Task *task = mu_tasks->start;
		while (task) {
			if (task == mu_tasks->current) {
				task = task->next;
				continue;
			}

			task->unblockTypeWith(IPC::WaitProcessCall::type(), mu_tasks->current->id);
			task = task->next;
		}

		mu_syscall->returnToNextTask();
		// Find the Task* which refers to mu_tasks->current, and get it to skip it.
		Tasks::Task **scanner = &mu_tasks->start;
		while (*scanner != mu_tasks->current) {
			scanner = &(*scanner)->next;
		}

		*scanner = (*scanner)->next;
		// Gone! XXX what happens when the last task exists!? Everything probably goes to hell ...
		// This should never happen because of the idle task; right?
	}

	void defer() {
		mu_syscall->returnToNextTask();
	}

	void *malloc(u32 n) {
		requirePrivilege(PRIV_MALLOC);

		ASSERT(mu_tasks->current->heap);
		return mu_tasks->current->heap->alloc(n);
	}

	void *mallocap(u32 n, phptr *p) {
		requirePrivilege(PRIV_MALLOC);
		requirePrivilege(PRIV_PHYSADDR);

		void *mem = mu_tasks->current->heap->allocAligned(n);
		*p = physAddr(mem);
		return mem;
	}

	phptr physAddr(void *ptr) {
		requirePrivilege(PRIV_PHYSADDR);

		Memory::Page *page = mu_tasks->current->pageDir->getPage(reinterpret_cast<u32>(ptr), false);
		if (!page) return 0;
		return page->pageAddress * 0x1000 + (reinterpret_cast<u32>(ptr) & 0xFFF);
	}

	void free(void *p) {
		requirePrivilege(PRIV_MALLOC);

		ASSERT(mu_tasks->current->heap);
		bool success = mu_tasks->current->heap->free(p);
		if (!success) {
			panic("free returned false");
		}
	}
}
#endif
