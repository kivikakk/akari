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

#include "MBRProto.hpp"
#include "../ATA/ATAProto.hpp"
#include "main.hpp"

void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, char *buffer);
void ata_read_data(u32 new_sector, u16 offset, u32 length, char *buffer);

master_boot_record_t hdd_mbr;
pid_t ata = 0;

extern "C" int start() {
	syscall_puts("MBR: waiting for ata\n");
	while (!ata)
		ata = syscall_processIdByName("system.io.ata");

	if (!syscall_registerName("system.io.mbr"))
		syscall_panic("MBR: could not register system.io.mbr");

	ata_read_data(0, 0, 512, reinterpret_cast<char *>(&hdd_mbr));
	if (hdd_mbr.signature != 0xAA55) syscall_panic("MBR: invalid MBR!\n");

	syscall_puts("MBR: entering loop\n");

	char *buffer = 0; u32 buffer_len = 0;

	while (true) {
		struct queue_item_info info = *syscall_probeQueue();

		// We assign (copy) this so we don't lose it after shiftQueue().
		u32 len = info.data_len;
		if (len > MBR_MAX_WILL_ALLOC) syscall_panic("MBR: given more data than would like to alloc");
		if (len > buffer_len) {
			if (buffer) delete [] buffer;
			buffer = new char[len];
			buffer_len = len;
		}

		syscall_readQueue(&info, buffer, 0, len);

		// Be sure to shift before going doing a call out that might want
		// to read from there ...
		syscall_shiftQueue(&info);

		if (buffer[0] == MBR_OP_READ) {
			MBROpRead op = *reinterpret_cast<MBROpRead *>(buffer);

			if (op.length > MBR_MAX_WILL_ALLOC) syscall_panic("MBR: request asked for more than we'd like to alloc");
			if (op.length > buffer_len) {
				delete [] buffer;
				buffer = new char[op.length];
				buffer_len = op.length;
			}

			if (op.length == 0) {
				syscall_puts("getting 0 bytes from sector ");
				syscall_putl(op.sector, 16);
				syscall_puts(", offset ");
				syscall_putl(op.offset, 16);
				syscall_puts("\n");
				syscall_panic("MBR: asked to get 0 bytes");
			}

			partition_read_data(op.partition_id, op.sector, op.offset, op.length, buffer);

			syscall_sendQueue(info.from, info.id, buffer, op.length);
		} else {
			syscall_panic("MBR: confused");
		}
	}

	syscall_panic("MBR: ran off its own loop!");
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
	
	ATAOpRead op = {
		ATA_OP_READ,
		new_sector,
		offset,
		length
	};

	u32 msg_id = syscall_sendQueue(ata, 0, reinterpret_cast<char *>(&op), sizeof(ATAOpRead));

	struct queue_item_info *info = syscall_probeQueueFor(msg_id);

	if (info->data_len != length) {
		syscall_puts("asked for ");
		syscall_putl(length, 16);
		syscall_puts(", got ");
		syscall_putl(info->data_len, 16);
		syscall_puts("\n");
		syscall_panic("MBR: ATA read not expected number of bytes back");
	}

	syscall_readQueue(info, buffer, 0, info->data_len);
	syscall_shiftQueue(info);
}
