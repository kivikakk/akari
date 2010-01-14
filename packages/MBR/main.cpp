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

void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, char *buffer);
void ata_read_data(u32 new_sector, u16 offset, u32 length, char *buffer);

master_boot_record_t hdd_mbr;
pid_t ata = 0;

extern "C" int start() {
	syscall_puts("MBR driver waiting for ata\n");
	while (!ata)
		ata = syscall_processIdByName("system.io.ata");

	if (!syscall_registerName("system.io.mbr"))
		syscall_panic("could not register system.io.mbr");

	ata_read_data(0, 0, 512, reinterpret_cast<char *>(&hdd_mbr));
	if (hdd_mbr.signature != 0xAA55) syscall_panic("Invalid MBR!\n");
	
	syscall_puts("MBR driver entering loop\n");

	char *buffer = 0; u32 buffer_len = 0;

	while (true) {
		struct queue_item_info info = *syscall_probeQueue();
		// We assign (copy) this so we don't lose it after shiftQueue().
		u32 len = info.data_len;
		if (len > MBR_MAX_WILL_ALLOC) syscall_panic("MBR driver given more data than would like to alloc");
		if (len > buffer_len) {
			if (buffer) delete [] buffer;
			buffer = new char[len];
			buffer_len = len;
		}

		syscall_readQueue(buffer, 0, len);

		// Be sure to shift before going doing a call out that might want
		// to read from there ...
		syscall_shiftQueue();

		if (buffer[0] == 0) {
			// Read

			u8 partition_id = buffer[1];
			u32 sector = buffer[2] << 24 | buffer[3] << 16 | buffer[4] << 8 | buffer[5];
			u16 offset = buffer[6] << 8 | buffer[7];
			u32 length = buffer[8] << 24 | buffer[9] << 16 | buffer[10] << 8 | buffer[11];

			if (length > MBR_MAX_WILL_ALLOC) syscall_panic("MBR request asked for more than we'd like to alloc");
			if (length > buffer_len) {
				delete [] buffer;
				buffer = new char[length];
				buffer_len = length;
			}

			partition_read_data(partition_id, sector, offset, length, buffer);

			syscall_sendQueue(info.from, info.id, buffer, length);
		} else {
			syscall_panic("MBR driver confused");
		}
	}

	syscall_panic("MBR ran off its own loop!");
	return 1;
}


void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, char *buffer) {
	u32 new_sector = hdd_mbr.partitions[partition_id].begin_disk_sector + sector;
	ata_read_data(new_sector, offset, length, buffer);
}

// TODO: place this outside.
void ata_read_data(u32 new_sector, u16 offset, u32 length, char *buffer) {
	// Request to ATA driver: u8 0x0 ('read'), u32 sector, u16 offset, u32 length
	// Total: 11 bytes
	
	char req[] = {
		0,
		(new_sector >> 24) & 0xFF, (new_sector >> 16) & 0xFF, (new_sector >> 8) & 0xFF, new_sector & 0xFF,
		(offset >> 8) & 0xFF, offset & 0xFF,
		(length >> 24) & 0xFF, (length >> 16) & 0xFF, (length >> 8) & 0xFF, length & 0xFF
	};

	// u32 id =
	syscall_sendQueue(ata, 0, req, 11);

	// TODO XXX: need to listen for replies to a certain message, not just the next one.

	struct queue_item_info *info = syscall_probeQueue();
	if (info->data_len != length) syscall_panic("MBR's ATA read: not expected number of bytes back?");

	syscall_readQueue(buffer, 0, info->data_len);
	syscall_shiftQueue();
}
