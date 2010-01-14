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
// along with Akari.  If not, see <http://www.gnu.org/licenses/

#include <UserCalls.hpp>
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>
#include <debug.hpp>

#include "main.hpp"
#include "../VFS/VFSProto.hpp"

void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, char *buffer);

fat_boot_record_t boot_record;
pid_t mbr = 0;

extern "C" int start() {
	syscall_puts("FAT driver waiting for mbr\n");
	while (!mbr)
		mbr = syscall_processIdByName("system.io.mbr");

	if (!syscall_registerName("system.io.fs.fat"))
		syscall_panic("could not register system.io.fs.fat");

	partition_read_data(0, 0, 0, sizeof(fat_boot_record_t), reinterpret_cast<char *>(&boot_record));

	syscall_puts("FAT driver entering loop\n");

	char *buffer = 0; u32 buffer_len = 0;

	while (true) {
		struct queue_item_info info = *syscall_probeQueue();
		u32 len = info.data_len;
		if (len > FAT_MAX_WILL_ALLOC) syscall_panic("FAT driver given more data than would like to alloc");
		if (len > buffer_len) {
			if (buffer) delete [] buffer;
			buffer = new char[len];
			buffer_len = len;
		}

		syscall_shiftQueue();

		if (buffer[0] == VFS_OP_READ) {
			// Read

			u8 partition_id = buffer[1];
			u32 sector = buffer[2] << 24 | buffer[3] << 16 | buffer[4] << 8 | buffer[5];
			u16 offset = buffer[6] << 8 | buffer[7];
			u32 length = buffer[8] << 24 | buffer[9] << 16 | buffer[10] << 8 | buffer[11];

			if (length > FAT_MAX_WILL_ALLOC) syscall_panic("FAT request asked for more than we'd like to alloc");
			if (length > buffer_len) {
				delete [] buffer;
				buffer = new char[length];
				buffer_len = length;
			}

			partition_read_data(partition_id, sector, offset, length, buffer);

			syscall_sendQueue(info.from, info.id, buffer, length);
		} else {
			syscall_panic("FAT driver confused");
		}
	}

	syscall_panic("FAT driver went off the edge?");
	return 1;
}


void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, char *buffer) {
	// Request to MBR driver: u8 0x0 ('read'), u8 parition_id, u32 sector, u16 offset, u32 length
	// Total: 12 bytes
	
	char req[] = {
		0,
		partition_id,
		(sector >> 24) & 0xFF, (sector >> 16) & 0xFF, (sector >> 8) & 0xFF, sector & 0xFF,
		(offset >> 8) & 0xFF, offset & 0xFF,
		(length >> 24) & 0xFF, (length >> 16) & 0xFF, (length >> 8) & 0xFF, length & 0xFF
	};

	// u32 id =
	syscall_sendQueue(mbr, 0, req, 12);;

	// TODO XXX: need to listen for replies to a certain message, not just the next one.

	struct queue_item_info *info = syscall_probeQueue();
	if (info->data_len != length) syscall_panic("FAT's MBR read: not expected number of bytes back?");

	syscall_readQueue(buffer, 0, info->data_len);
	syscall_shiftQueue();
}
