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

#ifndef __USER_IPC_HPP__
#define __USER_IPC_HPP__

#include <arch.hpp>
#include <UserGates.hpp>

struct queue_item_info {
	u32 id, timestamp, reply_to;
	u32 data_len;
};

#ifdef __AKARI_KERNEL__

namespace User {
namespace IPC {
	bool registerName(const char *name);

	bool registerStream(const char *name);
	u32 obtainStreamWriter(const char *name, const char *node, bool exclusive);
	u32 obtainStreamListener(const char *name, const char *node);
	u32 readStream(const char *name, const char *node, u32 listener, char *buffer, u32 n);
	u32 readStreamUnblock(const char *name, const char *node, u32 listener, char *buffer, u32 n);
	u32 writeStream(const char *name, const char *node, u32 writer, const char *buffer, u32 n);

	struct queue_item_info *probeQueue();
	struct queue_item_info *probeQueueUnblock();
	u32 readQueue(char *dest, u32 offset, u32 len);
	void shiftQueue();
	u32 sendQueue(const char *name, u32 reply_to, const char *buffer, u32 len);
}
}

#else

DECL_SYSCALL1(registerName, bool, const char *);

DECL_SYSCALL1(registerStream, bool, const char *);
DECL_SYSCALL3(obtainStreamWriter, u32, const char *, const char *, bool);
DECL_SYSCALL2(obtainStreamListener, u32, const char *, const char *);
DECL_SYSCALL5(readStream, u32, const char *, const char *, u32, char *, u32);
DECL_SYSCALL5(readStreamUnblock, u32, const char *, const char *, u32, char *, u32);
DECL_SYSCALL5(writeStream, u32, const char *, const char *, u32, const char *, u32);

DECL_SYSCALL0(probeQueue, struct queue_item_info *);
DECL_SYSCALL0(probeQueueUnblock, struct queue_item_info *);
DECL_SYSCALL3(readQueue, u32, char *, u32, u32);
DECL_SYSCALL0(shiftQueue, void);
DECL_SYSCALL4(sendQueue, u32, const char *, u32, const char *, u32);

#endif

#endif

