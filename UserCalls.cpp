#include <UserCalls.hpp>
#include <Akari.hpp>
#include <POSIX.hpp>
#include <Symbol.hpp>
#include <debug.hpp>
#include <Console.hpp>
#include <Tasks.hpp>
#include <Syscall.hpp>

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
		Symbol sName(name);
		if (Akari->tasks->registeredTasks->hasKey(sName))
			return false;

		(*Akari->tasks->registeredTasks)[sName] = Akari->tasks->current;
		Akari->tasks->current->registeredName = sName;
		return true;
	}

	bool registerNode(const char *node) {
		Symbol sNode(node);
		if (!Akari->tasks->current->registeredName) {
			// TODO: just kill the process, don't kill the system.
			// TODO: is this correct behaviour? Or could we have registered nodes
			// on no particular name? Why not?.. think about it.
			AkariPanic("name not registered - cannot register node");
		}

		if (Akari->tasks->current->nodesByName->hasKey(node)) {
			AkariPanic("node already registered - cannot register atop it");
		}

		Tasks::Task::Node *target = new Tasks::Task::Node();

		(*Akari->tasks->current->nodesByName)[sNode] = target;
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

	static inline Tasks::Task::Node *getNode(const char *name, const char *node) {
		Symbol sName(name), sNode(node);

		if (!Akari->tasks->registeredTasks->hasKey(sName))
			return 0;

		Tasks::Task *task = (*Akari->tasks->registeredTasks)[sName];
		if (!task->nodesByName->hasKey(sNode))
			return 0;

		return (*task->nodesByName)[sNode];
	}

	u32 obtainNodeWriter(const char *name, const char *node, bool exclusive) {
		Tasks::Task::Node *target = getNode(name, node);
		if (!target) return -1;
		return target->registerWriter(exclusive);
	}

	u32 obtainNodeListener(const char *name, const char *node) {
		Tasks::Task::Node *target = getNode(name, node);
		if (!target) return -1;
		return target->registerListener();
	}

	// Keeping in mind that `buffer''s data probably isn't asciz.
	u32 readNode(const char *name, const char *node, u32 listener, char *buffer, u32 n) {
		Tasks::Task::Node *target = getNode(name, node);
		if (!target || !target->hasListener(listener)) return -1;

		Tasks::Task::Node::Listener &l = target->getListener(listener);

		if (n == 0) return 0;

		u32 len = l.length();
		if (len == 0) return 0;
		
		if (len > n) len = n;
		POSIX::memcpy(buffer, l.view(), len);
		l.cut(len);

		return len;
	}

	u32 writeNode(const char *name, const char *node, u32 writer, const char *buffer, u32 n) {
		Tasks::Task::Node *target = getNode(name, node);
		if (!target || !target->hasWriter(writer)) return -1;

		// We do have a writer, so we can go ahead and write to all listeners.
		target->writeAllListeners(buffer, n);
		return n;		// what else?!
	}

	void defer() {
		Akari->syscall->returnToNextTask();
	}
}

