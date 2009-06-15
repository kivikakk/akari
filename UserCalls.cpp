#include <UserCalls.hpp>
#include <Akari.hpp>
#include <POSIX.hpp>

namespace User {
	void putc(char c) {
		Akari->Console->putChar(c);
	}

	void puts(const char *s) {
		Akari->Console->putString(s);
	}

	void putl(u32 n, u8 base) {
		Akari->Console->putInt(n, base);
	}
	u32 getProcessId() {
		return Akari->Task->current->id;
	}

	void irqWait() {
		if (Akari->Task->current->irqListenHits == 0) {
			Akari->Task->current->irqWaiting = true;

			Akari->Syscall->returnToNextTask();
			return;
		}

		Akari->Task->current->irqListenHits--;
	}

	void irqListen(u32 irq) {
		Akari->Task->current->irqListen = irq;
		Akari->Task->current->irqListenHits = 0;
	}


	void panic(const char *s) {
		// TODO: check permission. should be ring 1 or 0, but at least definitely not 3.
		// even ring 1 tasks should be able to die and come back up. that's the point, right?
		// so ring 1 tasks panicking should, like ring 3 ones, just be killed off. (and
		// the idea is that the system notes this and fixes it. yep) (TODO!)
		AkariPanic(s);
	}

	bool registerName(const char *name) {
		if (Akari->Task->registeredTasks->hasKey(name))
			return false;

		(*Akari->Task->registeredTasks)[name] = Akari->Task->current;
		Akari->Task->current->registeredName = name;
		return true;
	}

	bool registerNode(const char *name) {
		if (!Akari->Task->current->registeredName) {
			// TODO: just kill the process, don't kill the system.
			// TODO: is this correct behaviour? Or could we have registered nodes
			// on no particular name? Why not?.. think about it.
			AkariPanic("name not registered - cannot register node");
		}

		if (Akari->Task->current->nodesByName->hasKey(name)) {
			AkariPanic("node already registered - cannot register atop it");
		}

		AkariTaskSubsystem::Task::Node *node = new AkariTaskSubsystem::Task::Node();

		(*Akari->Task->current->nodesByName)[name] = node;
		return true;
	}
}

