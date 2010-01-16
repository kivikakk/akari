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

#include <stdio.hpp>
#include <UserCalls.hpp>
#include <arch.hpp>
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>

#include "../VFS/VFSProto.hpp"

pid_t vfs = 0;

extern "C" int start() {
	while (!vfs)
		vfs = processIdByName("system.io.vfs");

	{
		VFSOpAwaitRoot op = { VFS_OP_AWAIT_ROOT };
		u32 msg_id = sendQueue(vfs, 0, reinterpret_cast<u8 *>(&op), sizeof(VFSOpAwaitRoot));
		struct queue_item_info *info = probeQueueFor(msg_id);
		shiftQueue(info);
	}

	exit();
	return 0;
}

