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

#include <UserIPCQueue.hpp>
#include <Akari.hpp>
#include <POSIX.hpp>
#include <Symbol.hpp>
#include <debug.hpp>
#include <Syscall.hpp>
#include <BlockingCall.hpp>

namespace User {
namespace IPC {
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

		static Symbol type() { return Symbol("ProbeQueueCall"); }
		Symbol insttype() const { return type(); }
	
	protected:
		Tasks::Task *task;
	};

	static struct queue_item_info *probeQueue_impl(bool block) {
		ProbeQueueCall c(Akari->tasks->current);
		struct queue_item_info *r = reinterpret_cast<struct queue_item_info *>(c());
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
		return len;
	}

	void shiftQueue() {
		Akari->tasks->current->replyQueue->shift();
	}

	u32 sendQueue(const char *name, u32 reply_to, const char *buffer, u32 len) {
		Symbol sName(name);
		if (!Akari->tasks->registeredTasks->hasKey(sName))
			AkariPanic("cannot find task in sendQueue?");

		Tasks::Task *task = (*Akari->tasks->registeredTasks)[sName];

		u32 id = task->replyQueue->push_back(reply_to, buffer, len);
		task->unblockType(ProbeQueueCall::type());
		return id;
	}
}
}

DEFN_SYSCALL0(probeQueue, 19, struct queue_item_info *);
DEFN_SYSCALL0(probeQueueUnblock, 20, struct queue_item_info *);
DEFN_SYSCALL3(readQueue, 21, u32, char *, u32, u32);
DEFN_SYSCALL0(shiftQueue, 22, void);
DEFN_SYSCALL4(sendQueue, 23, u32, const char *, u32, const char *, u32);

