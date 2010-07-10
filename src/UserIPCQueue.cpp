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

#if defined(__AKARI_KERNEL)
#include <Akari.hpp>
#include <Console.hpp>
#include <Symbol.hpp>
#include <debug.hpp>
#include <Syscall.hpp>
#include <BlockingCall.hpp>

namespace User {
namespace IPC {
	class ProbeQueueCall : public BlockingCall {
	public:
		ProbeQueueCall(Tasks::Task *_task, u32 _reply_to): task(_task), reply_to(_reply_to)
		{ }

		u32 operator()() {
			Tasks::Task::Queue::Item *item;
			if (reply_to == 0)
				item = task->replyQueue->first();
			else
				item = task->replyQueue->itemByReplyTo(reply_to);

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

		bool unblockWith(u32 data) const {
			return reply_to == data;
		}

		static Symbol type() { return Symbol("ProbeQueueCall"); }
		Symbol insttype() const { return type(); }
	
	protected:
		Tasks::Task *task;
		u32 reply_to;
	};

	static struct queue_item_info *probeQueue_impl(bool block, u32 reply_to=0) {
		ProbeQueueCall c(Akari->tasks->current, reply_to);
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

	struct queue_item_info *probeQueueFor(u32 reply_to) {
		return probeQueue_impl(true, reply_to);
	}

	struct queue_item_info *probeQueueForUnblock(u32 reply_to) {
		return probeQueue_impl(false, reply_to);
	}

	u32 readQueue(struct queue_item_info *info, u8 *dest, u32 offset, u32 len) { 
		Tasks::Task::Queue::Item *item = Akari->tasks->current->replyQueue->itemById(info->id);
		if (!item) return 0;		// XXX error out!

		// If offset is out of bounds, just return.
		if (offset >= item->info.data_len) return item->info.id;

		// If the calculated end bound falls outside of the data,
		// just adjust the length to make it go to the end.
		if (offset + len > item->info.data_len) len = item->info.data_len - offset;

		// Sane offset and length. Off we go.
		memcpy(dest, item->data + offset, len);
		return len;
	}

	u8 *grabQueue(struct queue_item_info *info) {
		u8 *data = new u8[info->data_len];
		readQueue(info, data, 0, info->data_len);
		return data;
	}

	void shiftQueue(struct queue_item_info *info) {
		Tasks::Task::Queue::Item *item = Akari->tasks->current->replyQueue->itemById(info->id);
		Akari->tasks->current->replyQueue->remove(item);
	}

	u32 sendQueue(pid_t id, u32 reply_to, const u8 *buffer, u32 len) {
		Tasks::Task *task = Akari->tasks->getTaskById(id);
		u32 msg_id = task->replyQueue->push_back(Akari->tasks->current->id, reply_to, buffer, len);
		task->unblockTypeWith(ProbeQueueCall::type(), reply_to);
		return msg_id;
	}
}
}
#endif

