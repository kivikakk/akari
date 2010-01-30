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
#include <POSIX.hpp>
#include <Symbol.hpp>
#include <debug.hpp>
#include <Console.hpp>
#include <Tasks.hpp>
#include <Syscall.hpp>
#include <BlockingCall.hpp>
#include <Timer.hpp>

namespace User {
	void putc(char c) {
		Akari->console->putChar(c);
	}

	void puts(const char *s) {
		Akari->console->putString(s);
	}

	void putl(u32 n, u8 base) {
		Akari->console->putInt(n, base);
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
		Akari->tasks->current->irqListen = irq;
		Akari->tasks->current->irqListenHits = 0;
	}

	u32 ticks() {
		return AkariMicrokernelSwitches;
	}

	void panic(const char *s) {
		// TODO: check permission. should be ring 1 or 0, but at least definitely not 3.
		// even ring 1 tasks should be able to die and come back up. that's the point, right?
		// so ring 1 tasks panicking should, like ring 3 ones, just be killed off. (and
		// the idea is that the system notes this and fixes it. yep) (TODO!)
		AkariPanic(s);
	}

	void sysexit() {
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
		ASSERT(Akari->tasks->current->heap);
		return Akari->tasks->current->heap->alloc(n);
	}

	void free(void *p) {
		ASSERT(Akari->tasks->current->heap);
		// TODO! :-)
	}

	void *memcpy(void *dest, const void *src, u32 n) {
		char *w = static_cast<char *>(dest);
		const char *r = static_cast<const char *>(src);

		while (n--)
			*w++ = *r++;
		return dest;
	}

	char *strcpy(char *dest, const char *src) {
		return POSIX::strcpy(dest, src);
	}

	s32 strcmp(const char *s1, const char *s2) {
		return POSIX::strcmp(s1, s2);
	}

	s32 stricmp(const char *s1, const char *s2) {
		return POSIX::stricmp(s1, s2);
	}

	u32 strlen(const char *s) {
		return POSIX::strlen(s);
	}
	
	s32 strcmpn(const char *s1, const char *s2, u32 n) {
		return POSIX::strcmpn(s1, s2, n);
	}

	s32 strpos(const char *haystack, const char *needle) {
		return POSIX::strpos(haystack, needle);
	}
}
#endif
