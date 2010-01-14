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

	void irqWait() {
		if (Akari->tasks->current->irqListenHits == 0) {
			Akari->tasks->current->irqWaiting = true;

			Akari->syscall->returnToNextTask();
			return;
		}

		Akari->tasks->current->irqListenHits--;
	}

	void irqListen(u32 irq) {
		Akari->tasks->current->irqListen = irq;
		Akari->tasks->current->irqListenHits = 0;
	}


	void panic(const char *s) {
		// TODO: check permission. should be ring 1 or 0, but at least definitely not 3.
		// even ring 1 tasks should be able to die and come back up. that's the point, right?
		// so ring 1 tasks panicking should, like ring 3 ones, just be killed off. (and
		// the idea is that the system notes this and fixes it. yep) (TODO!)
		AkariPanic(s);
	}

	void exit() {
		Akari->syscall->returnToNextTask();
		// Find the Task* which refers to Akari->tasks->current, and get it to skip it.
		Tasks::Task **scanner = &Akari->tasks->start;
		while (*scanner != Akari->tasks->current) {
			scanner = &(*scanner)->next;
		}

		Akari->console->putString("exit(): 0x");
		Akari->console->putInt(reinterpret_cast<u32>(*scanner), 16);
		Akari->console->putString(" -> 0x");
		Akari->console->putInt(reinterpret_cast<u32>((*scanner)->next), 16);
		Akari->console->putString("\n");

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
}
#endif
