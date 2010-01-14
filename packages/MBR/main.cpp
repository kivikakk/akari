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

typedef struct
{
	u8 bootable;	/* 0x80 if bootable, 0 otherwise */
	u8 begin_head;
	unsigned begin_cylinderhi : 2; /* 2 high bits of start cylinder */
	unsigned begin_sector : 6;
	u8 begin_cylinderlo; /* low bits of start cylinder */
	u8 system_id;
	u8 end_head;
	unsigned end_cylinderhi : 2; /* 2 high bits of end cylinder */
	unsigned end_sector : 6;
	u8 end_cylinderlo; /* low bits of end cylinder */
	u32 begin_disk_sector;
	u32 sector_count;
} __attribute__ ((__packed__)) primary_partition_t;

typedef struct
{
	u8 boot[446];
	primary_partition_t partitions[4];
	u16 signature; /* should always be 0xaa55 */
} __attribute__ ((__packed__)) master_boot_record_t;

void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, char *buffer);
void ata_read_data(u32 new_sector, u16 offset, u32 length, char *buffer);

void MBRProcess() {
	if (!syscall_registerName("system.io.mbr"))
		syscall_panic("could not register system.io.mbr");

	syscall_puts("MBR driver entering loop\n");
#define MBR_BUFFER 10240
	char buffer[MBR_BUFFER];

	while (true) {
		struct queue_item_info info = *syscall_probeQueue();
		// We assign (copy) this so we don't lose it after shiftQueue().

		u32 len = info.data_len;
		if (len > MBR_BUFFER) syscall_panic("MBR buffer overflow");
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

			if (length > MBR_BUFFER) syscall_panic("Static buffer in MBR too small for req'd amount");

			partition_read_data(partition_id, sector, offset, length, buffer);

			syscall_sendQueue(info.from, info.id, buffer, length);
		} else {
			syscall_panic("MBR driver confused");
		}
	}
}


void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, char *buffer)
{
	// This is being REALLY wasteful by re-requesting the MBR EVERY REQUEST,
	// but what can you do?
	master_boot_record_t hdd_mbr;
	ata_read_data(0, 0, 512, reinterpret_cast<char *>(&hdd_mbr));

	if (hdd_mbr.signature != 0xAA55) syscall_panic("Invalid MBR!\n");
	
	u32 new_sector = hdd_mbr.partitions[partition_id].begin_disk_sector + sector;

	ata_read_data(new_sector, offset, length, buffer);
}

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
	syscall_sendQueue(syscall_processIdByName("system.io.ata"), 0, req, 11);

	// TODO XXX: need to listen for replies to a certain message, not just the next one.

	struct queue_item_info *info = syscall_probeQueue();
	if (info->data_len != length) syscall_panic("MBR's ATA read: not expected number of bytes back?");

	syscall_readQueue(buffer, 0, info->data_len);
	syscall_shiftQueue();
}
