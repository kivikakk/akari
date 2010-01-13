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
	u32 id, timestamp, reply_to;
	u32 data_len;
};

#ifdef __AKARI_KERNEL__

#include <Tasks.hpp>

namespace User {
namespace IPC {
	struct queue_item_info *probeQueue();
	struct queue_item_info *probeQueueUnblock();
	u32 readQueue(char *dest, u32 offset, u32 len);
	void shiftQueue();
	u32 sendQueue(const char *name, u32 reply_to, const char *buffer, u32 len);
}
}

#else

DECL_SYSCALL0(probeQueue, struct queue_item_info *);
DECL_SYSCALL0(probeQueueUnblock, struct queue_item_info *);
DECL_SYSCALL3(readQueue, u32, char *, u32, u32);
DECL_SYSCALL0(shiftQueue, void);
DECL_SYSCALL4(sendQueue, u32, const char *, u32, const char *, u32);

#endif

#endif

