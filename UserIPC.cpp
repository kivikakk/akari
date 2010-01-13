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
#include <Akari.hpp>
#include <POSIX.hpp>
#include <Symbol.hpp>
#include <debug.hpp>
#include <Console.hpp>
#include <Syscall.hpp>
#include <BlockingCall.hpp>

namespace User {
namespace IPC {
	pid_t processId() {
		return Akari->tasks->current->id;
	}

	pid_t processIdByName(const char *name) {
		Symbol sName(name);
		if (!Akari->tasks->registeredTasks->hasKey(sName))
			return 0;

		Tasks::Task *task = (*Akari->tasks->registeredTasks)[sName];
		return task->id;
	}

	bool registerName(const char *name) {
		Symbol sName(name);
		if (Akari->tasks->registeredTasks->hasKey(sName))
			return false;

		(*Akari->tasks->registeredTasks)[sName] = Akari->tasks->current;
		Akari->tasks->current->registeredName = sName;
		return true;
	}

	bool registerStream(const char *node) {
		Symbol sNode(node);
		if (Akari->tasks->current->streamsByName->hasKey(node)) {
			AkariPanic("node already registered - cannot register atop it");
		}

		Tasks::Task::Stream *target = new Tasks::Task::Stream();

		(*Akari->tasks->current->streamsByName)[sNode] = target;
		return true;
	}

	static inline Tasks::Task::Stream *getStream(pid_t id, const char *node) {
		Symbol sNode(node);
		Tasks::Task *task = Akari->tasks->getTaskById(id);
		if (!task || !task->streamsByName->hasKey(sNode))
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

	// ReadStreamCall's implementation

	ReadStreamCall::ReadStreamCall(pid_t id, const char *node, u32 listener, char *buffer, u32 n):
		_listener(&getStream(id, node)->getListener(listener)), _buffer(buffer), _n(n)
	{ }

	Tasks::Task::Stream::Listener *ReadStreamCall::getListener() const {
		return _listener;
	}

	u32 ReadStreamCall::operator ()() {
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

DEFN_SYSCALL0(processId, 24, pid_t);
DEFN_SYSCALL1(processIdByName, 25, pid_t, const char *);
DEFN_SYSCALL1(registerName, 12, bool, const char *);

DEFN_SYSCALL1(registerStream, 13, bool, const char *);
DEFN_SYSCALL3(obtainStreamWriter, 14, u32, pid_t, const char *, bool);
DEFN_SYSCALL2(obtainStreamListener, 15, u32, pid_t, const char *);
DEFN_SYSCALL5(readStream, 16, u32, pid_t, const char *, u32, char *, u32);
DEFN_SYSCALL5(readStreamUnblock, 17, u32, pid_t, const char *, u32, char *, u32);
DEFN_SYSCALL5(writeStream, 18, u32, pid_t, const char *, u32, const char *, u32);

