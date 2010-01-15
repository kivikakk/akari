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

void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, u8 *buffer);
void ata_read_data(u32 new_sector, u16 offset, u32 length, u8 *buffer);

master_boot_record_t hdd_mbr;
pid_t ata = 0;

extern "C" int start() {
	while (!ata)
		ata = processIdByName("system.io.ata");

	if (!registerName("system.io.mbr"))
		panic("MBR: could not register system.io.mbr");

	ata_read_data(0, 0, 512, reinterpret_cast<u8 *>(&hdd_mbr));
	if (hdd_mbr.signature != 0xAA55) panic("MBR: invalid MBR!\n");

	printf("[MBR] ");

	while (true) {
		struct queue_item_info info = *probeQueue();
		u8 *request = grabQueue(&info);
		shiftQueue(&info);

		if (request[0] == MBR_OP_READ) {
			MBROpRead *op = reinterpret_cast<MBROpRead *>(request);


			u8 *buffer = new u8[op->length];
			partition_read_data(op->partition_id, op->sector, op->offset, op->length, buffer);
			sendQueue(info.from, info.id, buffer, op->length);
			delete [] buffer;
		} else {
			panic("MBR: confused");
		}

		delete [] request;
	}

	panic("MBR: ran off its own loop!");
	return 1;
}


void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, u8 *buffer) {
	u32 new_sector = hdd_mbr.partitions[partition_id].begin_disk_sector + sector;
	ata_read_data(new_sector, offset, length, buffer);
}

// TODO: place this outside.
void ata_read_data(u32 new_sector, u16 offset, u32 length, u8 *buffer) {
	// Request to ATA driver: u8 0x0 ('read'), u32 sector, u16 offset, u32 length
	// Total: 11 bytes
	
	ATAOpRead op = {
		ATA_OP_READ,
		new_sector,
		offset,
		length
	};

	u32 msg_id = sendQueue(ata, 0, reinterpret_cast<u8 *>(&op), sizeof(ATAOpRead));

	struct queue_item_info *info = probeQueueFor(msg_id);

	if (info->data_len != length) {
		printf("asked for 0x%x, got 0x%x\n", length, info->data_len);
		panic("MBR: ATA read not expected number of bytes back");
	}

	readQueue(info, buffer, 0, info->data_len);
	shiftQueue(info);
}
