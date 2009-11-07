#include <UserCalls.hpp>
#include <Akari.hpp>
#include <POSIX.hpp>
#include <Symbol.hpp>
#include <debug.hpp>
#include <Console.hpp>
#include <Tasks.hpp>
#include <Syscall.hpp>
#include <BlockingCall.hpp>

namespace User {
namespace IPC {
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

	class ReadCall : public BlockingCall {
	public:
		ReadCall(const char *name, const char *node, u32 listener, char *buffer, u32 n):
			_listener(&getStream(name, node)->getListener(listener)), _buffer(buffer), _n(n)
		{ }

		Tasks::Task::Stream::Listener *getListener() const {
			return _listener;
		}

		u32 operator ()() {
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

	protected:
		Tasks::Task::Stream::Listener *_listener;
		char *_buffer;
		u32 _n;
	};

	// Stream reading calls (can block).

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
}
}
