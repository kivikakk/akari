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

	bool registerStream(const char *name) {
		Symbol sNode(name);
		if (!Akari->tasks->current->registeredName) {
			// TODO: just kill the process, don't kill the system.
			// TODO: is this correct behaviour? Or could we have registered nodes
			// on no particular name? Why not?.. think about it.
			AkariPanic("name not registered - cannot register node");
		}

		if (Akari->tasks->current->streamsByName->hasKey(name)) {
			AkariPanic("node already registered - cannot register atop it");
		}

		Tasks::Task::Stream *target = new Tasks::Task::Stream();

		(*Akari->tasks->current->streamsByName)[sNode] = target;
		return true;
	}

	bool registerQueue(const char *name) {
		Symbol sNode(name);
		if (!Akari->tasks->current->registeredName) {
			// TODO
			AkariPanic("name not registered - cannot register node");
		}

		if (Akari->tasks->current->streamsByName->hasKey(name)) {
			AkariPanic("node already registered - cannot register atop it");
		}

		Tasks::Task::Queue *target = new Tasks::Task::Queue();

		(*Akari->tasks->current->queuesByName)[sNode] = target;
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

	static inline Tasks::Task::Stream *getStream(const char *name, const char *node) {
		Symbol sName(name), sNode(node);

		if (!Akari->tasks->registeredTasks->hasKey(sName))
			return 0;

		Tasks::Task *task = (*Akari->tasks->registeredTasks)[sName];
		if (!task->streamsByName->hasKey(sNode))
			return 0;

		return (*task->streamsByName)[sNode];
	}

	u32 obtainStreamWriter(const char *name, const char *node, bool exclusive) {
		Tasks::Task::Stream *target = getStream(name, node);
		if (!target) return -1;
		return target->registerWriter(exclusive);
	}

	u32 obtainStreamListener(const char *name, const char *node) {
		Tasks::Task::Stream *target = getStream(name, node);
		if (!target) return -1;
		return target->registerListener();
	}

	// Keeping in mind that `buffer''s data probably isn't asciz.
	u32 readStream_impl(const char *name, const char *node, u32 listener, char *buffer, u32 n, bool block) {
		ReadCall c(name, node, listener, buffer, n);
		u32 r = c();
		if (!block || !c.shallBlock())
			return r;
	
		// block && r.shallBlock()
		// Block until such time as some data is available.
		Tasks::Task::Stream::Listener *l = c.getListener();

		Akari->tasks->current->userWaiting = true;
		Akari->tasks->current->userCall = new ReadCall(c);
		l->hook(Akari->tasks->current);
		Akari->syscall->returnToNextTask();
		return 0;
	}

	u32 readStream(const char *name, const char *node, u32 listener, char *buffer, u32 n) {
		return readStream_impl(name, node, listener, buffer, n, true);
	}

	u32 readStreamUnblock(const char *name, const char *node, u32 listener, char *buffer, u32 n) {
		return readStream_impl(name, node, listener, buffer, n, false);
	}
	u32 writeStream(const char *name, const char *node, u32 writer, const char *buffer, u32 n) {
		Tasks::Task::Stream *target = getStream(name, node);
		if (!target || !target->hasWriter(writer)) return -1;

		// We do have a writer, so we can go ahead and write to all listeners.
		target->writeAllListeners(buffer, n);
		return n;		// what else?!
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
		char *w = (char *)dest;
		const char *r = (const char *)src;

		while (n--)
			*w++ = *r++;
		return dest;
	}

    ReadCall::ReadCall(const char *name, const char *node, u32 listener, char *buffer, u32 n):
		_listener(&getStream(name, node)->getListener(listener)), _buffer(buffer), _n(n)
	{ }

	Tasks::Task::Stream::Listener *ReadCall::getListener() const {
		return _listener;
	}

	u32 ReadCall::operator ()() {
		if (_n == 0) {
			_wontBlock();
			return 0;
		}

		u32 len = _listener->length();
		if (len == 0) {
			_willBlock();
			return 0;
		}
		
		if (len > _n) len = _n;
		POSIX::memcpy(_buffer, _listener->view(), len);
		_listener->cut(len);

		_wontBlock();
		return len;
	}
}

