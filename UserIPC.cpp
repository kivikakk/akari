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

	class ReadStreamCall : public BlockingCall {
	public:
		ReadStreamCall(const char *name, const char *node, u32 listener, char *buffer, u32 n):
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
	static u32 readStream_impl(const char *name, const char *node, u32 listener, char *buffer, u32 n, bool block) {
		ReadStreamCall c(name, node, listener, buffer, n);
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

	class ProbeQueueCall : public BlockingCall {
	public:
		ProbeQueueCall(Tasks::Task *_task): task(_task)
		{ }

		u32 operator()() {
			Tasks::Task::Queue::Item *item = task->replyQueue->first();
			if (!item) {
				_willBlock();
				return 0;
			}

			_wontBlock();

			// Making sure it's castable to what we're actually wanting before
			// throwing it as a u32.  I'm not sure if this is even necessary
			// or correct, but at least it won't break it (static_cast goes away
			// totally at compile-time ...).
			return (u32)static_cast<struct queue_item_info *>(&item->info);
		}
	
	protected:
		Tasks::Task *task;
	};

	static struct queue_item_info *probeQueue_impl(bool block) {
		ProbeQueueCall c(Akari->tasks->current);
		struct queue_item_info *r = (struct queue_item_info *)c();
		if (!block || !c.shallBlock())
			return r;

		Akari->tasks->current->userWaiting = true;
		Akari->tasks->current->userCall = new ProbeQueueCall(c);
		Akari->syscall->returnToNextTask();
		return 0;
	}

	struct queue_item_info *probeQueue() {
		return probeQueue_impl(true);
	}

	struct queue_item_info *probeQueueUnblock() {
		return probeQueue_impl(false);
	}

	u32 readQueue(char *dest, u32 offset, u32 len) { 
		Tasks::Task::Queue::Item *item = Akari->tasks->current->replyQueue->first();
		if (!item) return 0;		// XXX error out!

		// If offset is out of bounds, just return.
		if (offset >= item->info.data_len) return item->info.id;

		// If the calculated end bound falls outside of the data,
		// just adjust the length to make it go to the end.
		if (offset + len > item->info.data_len) len = item->info.data_len - offset;

		// Sane offset and length. Off we go.
		POSIX::memcpy(dest, item->data + offset, len);

		return item->info.id;
	}

	void shiftQueue() {
		Akari->tasks->current->replyQueue->shift();
	}

	u32 sendQueue(const char *name, u32 reply_to, const char *buffer, u32 len) {
		Symbol sName(name);
		if (!Akari->tasks->registeredTasks->hasKey(sName))
			AkariPanic("cannot find task in sendQueue?");

		Tasks::Task *task = (*Akari->tasks->registeredTasks)[sName];

		return task->replyQueue->push_back(reply_to, buffer, len);
	}
}
}

DEFN_SYSCALL1(registerName, 12, u32, const char *);

DEFN_SYSCALL1(registerStream, 13, u32, const char *);
DEFN_SYSCALL3(obtainStreamWriter, 14, u32, const char *, const char *, bool);
DEFN_SYSCALL2(obtainStreamListener, 15, u32, const char *, const char *);
DEFN_SYSCALL5(readStream, 16, u32, const char *, const char *, u32, char *, u32);
DEFN_SYSCALL5(readStreamUnblock, 17, u32, const char *, const char *, u32, char *, u32);
DEFN_SYSCALL5(writeStream, 18, u32, const char *, const char *, u32, const char *, u32);

DEFN_SYSCALL0(probeQueue, 19, struct queue_item_info *);
DEFN_SYSCALL0(probeQueueUnblock, 20, struct queue_item_info *);
DEFN_SYSCALL3(readQueue, 21, u32, char *, u32, u32);
DEFN_SYSCALL0(shiftQueue, 22, void);
DEFN_SYSCALL4(sendQueue, 23, u32, const char *, u32, const char *, u32);

