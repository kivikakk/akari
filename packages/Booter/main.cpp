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
#include <fs.hpp>
#include <UserCalls.hpp>
#include <arch.hpp>
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>
#include <UserProcess.hpp>

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

	FILE *kb = fopen("/Kb", "r");
	u32 kb_len = flen(kb);
	u8 *kb_image = new u8[kb_len];
	fread(kb_image, kb_len, 1, kb);
	fclose(kb);

	pid_t fr = spawn("kb", kb_image, kb_len);
	printf("Booter: just spawned something, I got %x\n", fr);

	exit();
	return 0;
}

