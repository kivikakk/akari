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

#ifndef __USER_IPC_QUEUE_HPP__
#define __USER_IPC_QUEUE_HPP__

#include <arch.hpp>
#include <UserGates.hpp>

struct queue_item_info {
	u32 id, timestamp;
	pid_t from; u32 reply_to;
	u32 data_len;
};

#if defined(__AKARI_KERNEL)

#include <Tasks.hpp>

namespace User {
namespace IPC {
	struct queue_item_info *probeQueue();
	struct queue_item_info *probeQueueUnblock();
	struct queue_item_info *probeQueueFor(u32 reply_to);
	struct queue_item_info *probeQueueForUnblock(u32 reply_to);
	u32 readQueue(struct queue_item_info *info, u8 *dest, u32 offset, u32 len);
	u8 *grabQueue(struct queue_item_info *info);
	void shiftQueue(struct queue_item_info *info);
	u32 sendQueue(pid_t id, u32 reply_to, const u8 *buffer, u32 len);
}
}

#elif defined(__AKARI_LINKAGE)

DEFN_SYSCALL0(probeQueue, 19, struct queue_item_info *)
DEFN_SYSCALL0(probeQueueUnblock, 20, struct queue_item_info *)
DEFN_SYSCALL1(probeQueueFor, 26, struct queue_item_info *, u32)
DEFN_SYSCALL1(probeQueueForUnblock, 27, struct queue_item_info *, u32)
DEFN_SYSCALL4(readQueue, 21, u32, struct queue_item_info *, u8 *, u32, u32)
DEFN_SYSCALL1(grabQueue, 31, u8 *, struct queue_item_info *)
DEFN_SYSCALL1(shiftQueue, 22, void, struct queue_item_info *)
DEFN_SYSCALL4(sendQueue, 23, u32, pid_t, u32, const u8 *, u32)

#else

DECL_SYSCALL0(probeQueue, struct queue_item_info *);
DECL_SYSCALL0(probeQueueUnblock, struct queue_item_info *);
DECL_SYSCALL1(probeQueueFor, struct queue_item_info *, u32);
DECL_SYSCALL1(probeQueueForUnblock, struct queue_item_info *, u32);
DECL_SYSCALL4(readQueue, u32, struct queue_item_info *, u8 *, u32, u32);
DECL_SYSCALL1(grabQueue, u8 *, struct queue_item_info *);
DECL_SYSCALL1(shiftQueue, void, struct queue_item_info *);
DECL_SYSCALL4(sendQueue, u32, pid_t id, u32, const u8 *, u32);

#endif

#endif

