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

#include <UserIPC.hpp>

#if defined(__AKARI_KERNEL)
#include <Akari.hpp>
#include <Symbol.hpp>
#include <debug.hpp>
#include <Console.hpp>
#include <Syscall.hpp>
#include <BlockingCall.hpp>

namespace User {
namespace IPC {
	int getProcessList(process_info_t **info) {
		int pcount = 1;
		Tasks::Task *task = Akari->tasks->start;
		while (task->next) {
			task = task->next; ++pcount;
		}

		*info = reinterpret_cast<process_info_t *>(
				Akari->tasks->current->heap->alloc(sizeof(process_info_t) * pcount));

		int index = 0;
		task = Akari->tasks->start;
		while (index < pcount) {
			process_info_t *i = &((*info)[index]);

			i->pid = task->id;

			i->flags  = task->userWaiting ? PROCESS_FLAG_BLOCKING : 0;
			i->flags |= task->irqListen ? PROCESS_FLAG_IRQ_LISTEN : 0;
			i->flags |= task == Akari->tasks->current ? PROCESS_FLAG_CURRENT : 0;

			i->name = 0;
			if (task->name.length() > 0) {
				i->name = reinterpret_cast<char *>(
						Akari->tasks->current->heap->alloc(task->name.length() + 1));
				strcpy(i->name, task->name.c_str());
			}

			i->registeredName = 0;
			if (task->registeredName.c_str() != 0) {
				i->registeredName = reinterpret_cast<char *>(
						Akari->tasks->current->heap->alloc(
							strlen(task->registeredName.c_str()) + 1));
				strcpy(i->registeredName, task->registeredName.c_str());
			}

			i->cpl = task->cpl;

			task = task->next;
			++index;
		}

		return pcount;
	}

	int waitProcess(pid_t pid) {
		WaitProcessCall c(pid);
		int status = c();
		if (!c.shallBlock())
			return status;

		Akari->tasks->current->userWaiting = true;
		Akari->tasks->current->userCall = new WaitProcessCall(c);
		Akari->syscall->returnToNextTask();
		return 0;
	}

	pid_t processId() {
		return Akari->tasks->current->id;
	}

	class ProcessIdByNameCall : public BlockingCall {
	public:
		ProcessIdByNameCall(const char *name): name(name)
		{ }

		u32 operator()() {
			if (Akari->tasks->registeredTasks.find(name) == Akari->tasks->registeredTasks.end()) {
				_willBlock();
				return 0;
			}

			_wontBlock();
			return static_cast<u32>(Akari->tasks->registeredTasks[name]->id);
		}

		const Symbol &getName() const { return name; }

		static Symbol type() { return Symbol("ProcessIdByNameCall"); }
		Symbol insttype() const { return type(); }

	protected:
		Symbol name;
	};

	pid_t processIdByName_impl(const char *name, bool block) {
		ProcessIdByNameCall c(name);
		pid_t r = static_cast<pid_t>(c());
		if (!block || !c.shallBlock())
			return r;

		Akari->tasks->current->userWaiting = true;
		Akari->tasks->current->userCall = new ProcessIdByNameCall(c);
		Akari->syscall->returnToNextTask();
		return 0;
	}

	pid_t processIdByName(const char *name) {
		return processIdByName_impl(name, false);
	}

	pid_t processIdByNameBlock(const char *name) {
		return processIdByName_impl(name, true);
	}

	bool registerName(const char *name) {
		Symbol sName(name);

		if (Akari->tasks->registeredTasks.find(sName) != Akari->tasks->registeredTasks.end())
			return false;

		Akari->tasks->registeredTasks[sName] = Akari->tasks->current;
		Akari->tasks->current->registeredName = sName;

		for (Tasks::Task *task = Akari->tasks->start; (task); task = task->next) {
			if (task->userWaiting && task->userCall && task->userCall->insttype() == ProcessIdByNameCall::type()) {
				if (static_cast<ProcessIdByNameCall *>(task->userCall)->getName() == sName)
					task->userWaiting = false;
			}
		}

		return true;
	}

	bool registerStream(const char *node) {
		Symbol sNode(node);
		if (Akari->tasks->current->streamsByName->find(node) != Akari->tasks->current->streamsByName->end()) {
			AkariPanic("node already registered - cannot register atop it");
		}

		Tasks::Task::Stream *target = new Tasks::Task::Stream();

		(*Akari->tasks->current->streamsByName)[sNode] = target;
		return true;
	}

	static inline Tasks::Task::Stream *getStream(pid_t id, const char *node) {
		Symbol sNode(node);
		Tasks::Task *task = Akari->tasks->getTaskById(id);
		if (!task || (task->streamsByName->find(sNode) == task->streamsByName->end()))
			return 0;

		return (*task->streamsByName)[sNode];
	}

	u32 obtainStreamWriter(pid_t id, const char *node, bool exclusive) {
		Tasks::Task::Stream *target = getStream(id, node);
		if (!target) return -1;
		return target->registerWriter(exclusive);
	}

	u32 obtainStreamListener(pid_t id, const char *node) {
		Tasks::Task::Stream *target = getStream(id, node);
		if (!target) return -1;
		return target->registerListener();
	}

	// WaitProcessCall's implementation
	
	WaitProcessCall::WaitProcessCall(pid_t pid): _pid(pid)
	{ }

	WaitProcessCall::WaitProcessCall(const WaitProcessCall &c):
		_pid(c._pid)
	{ }

	u32 WaitProcessCall::operator ()() {
		Tasks::Task *task = Akari->tasks->start;
		bool running = false;

		while (task) {
			if (task->id == _pid) {
				running = true;
				break;
			}
			task = task->next;
		}

		if (running) {
			_willBlock();
			return 0;
		}

		_wontBlock();
		return 0;		// (arlen): this is NOT a return code.
	}

	bool WaitProcessCall::unblockWith(u32 data) const {
		return _pid == data;
	}

	Symbol WaitProcessCall::type() { return Symbol("WaitProcessCall"); }
	Symbol WaitProcessCall::insttype() const { return type(); }

	// ReadStreamCall's implementation

	ReadStreamCall::ReadStreamCall(pid_t id, const char *node, u32 listener, char *buffer, u32 n):
		_listener(&getStream(id, node)->getListener(listener)), _buffer(buffer), _n(n)
	{ }

	ReadStreamCall::ReadStreamCall(const ReadStreamCall &r):
		_listener(r._listener), _buffer(r._buffer), _n(r._n)
	{ }

	/*ReadStreamCall &ReadStreamCall::operator =(const ReadStreamCall &r) {
		_listener = r._listener;
		_buffer = r._buffer;
		_n = r._n;
		return *this;
	}*/

	Tasks::Task::Stream::Listener *ReadStreamCall::getListener() const {
		return _listener;
	}

	u32 ReadStreamCall::operator()() {
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
		memcpy(_buffer, _listener->view(), len);
		_listener->cut(len);

		_wontBlock();
		return len;
	}

	Symbol ReadStreamCall::type() { return Symbol("ReadStreamCall"); }
	Symbol ReadStreamCall::insttype() const { return type(); }

	// Stream reading calls (can block).

	// Keeping in mind that `buffer''s data probably isn't asciz.
	static u32 readStream_impl(pid_t id, const char *node, u32 listener, char *buffer, u32 n, bool block) {
		ReadStreamCall c(id, node, listener, buffer, n);
		u32 r = c();
		if (!block || !c.shallBlock())
			return r;
	
		// block && r.shallBlock()
		// Block until such time as some data is available.
		Tasks::Task::Stream::Listener *l = c.getListener();

		Akari->tasks->current->userWaiting = true;
		Akari->tasks->current->userCall = new ReadStreamCall(c);
		l->hook(Akari->tasks->current);
		Akari->syscall->returnToNextTask();
		return 0;
	}

	u32 readStream(pid_t id, const char *node, u32 listener, char *buffer, u32 n) {
		return readStream_impl(id, node, listener, buffer, n, true);
	}

	u32 readStreamUnblock(pid_t id, const char *node, u32 listener, char *buffer, u32 n) {
		return readStream_impl(id, node, listener, buffer, n, false);
	}

	u32 writeStream(pid_t id, const char *node, u32 writer, const char *buffer, u32 n) {
		Tasks::Task::Stream *target = getStream(id, node);
		if (!target || !target->hasWriter(writer)) return -1;

		// We do have a writer, so we can go ahead and write to all listeners.
		target->writeAllListeners(buffer, n);
		return n;		// what else?!
	}
}
}
#endif

