#include <UserCalls.hpp>
#include <Akari.hpp>
#include <POSIX.hpp>

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

	bool registerName(const char *name) {
		if (Akari->tasks->registeredTasks->hasKey(name))
			return false;

		(*Akari->tasks->registeredTasks)[name] = Akari->tasks->current;
		Akari->tasks->current->registeredName = name;
		return true;
	}

	bool registerNode(const char *name) {
		if (!Akari->tasks->current->registeredName) {
			// TODO: just kill the process, don't kill the system.
			// TODO: is this correct behaviour? Or could we have registered nodes
			// on no particular name? Why not?.. think about it.
			AkariPanic("name not registered - cannot register node");
		}

		if (Akari->tasks->current->nodesByName->hasKey(name)) {
			AkariPanic("node already registered - cannot register atop it");
		}

		Tasks::Task::Node *node = new Tasks::Task::Node();

		(*Akari->tasks->current->nodesByName)[name] = node;
		return true;
	}

	void exit() {
		Akari->syscall->returnToNextTask();
		// Find the Task* which refers to Akari->tasks->current, and get it to skip it.
		Tasks::Task **scanner = &Akari->tasks->start;
		while (*scanner != Akari->tasks->current) {
			scanner = &(*scanner)->next;
		}

		Akari->console->putString("exit(): 0x");
		Akari->console->putInt((u32)*scanner, 16);
		Akari->console->putString(" -> 0x");
		Akari->console->putInt((u32)(*scanner)->next, 16);
		Akari->console->putString("\n");

		*scanner = (*scanner)->next;
		// Gone! XXX what happens when the last task exists!? Everything probably goes to hell ...
	}
}

