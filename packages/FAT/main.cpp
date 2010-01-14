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

bool init();
void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, char *buffer);

pid_t mbr = 0;

static fat_boot_record_t boot_record;
static u8 *fat_data;

static u32 root_dir_sectors, first_data_sector, first_fat_sector, data_sectors, total_clusters, fat_sectors;
static u8 fat_type, fat32esque;
static u32 root_cluster;

extern "C" int start() {
	syscall_puts("FAT: waiting for mbr\n");
	while (!mbr)
		mbr = syscall_processIdByName("system.io.mbr");

	if (!init()) {
		syscall_puts("FAT: calling it quits in init");
		syscall_exit();
	}

	if (!syscall_registerName("system.io.fs.fat"))
		syscall_panic("FAT: could not register system.io.fs.fat");

	syscall_puts("FAT: entering loop\n");

	char *buffer = 0; u32 buffer_len = 0;

	while (true) {
		struct queue_item_info info = *syscall_probeQueue();
		syscall_puts("FAT: got request\n");

		u32 len = info.data_len;
		if (len > FAT_MAX_WILL_ALLOC) syscall_panic("FAT: given more data than would like to alloc");
		if (len > buffer_len) {
			if (buffer) delete [] buffer;
			buffer = new char[len];
			buffer_len = len;
		}

		syscall_readQueue(&info, buffer, 0, len);
		syscall_shiftQueue(&info);

		if (buffer[0] == VFS_OP_READ) {
			// Read

			u8 partition_id = buffer[1];
			u32 sector = buffer[2] << 24 | buffer[3] << 16 | buffer[4] << 8 | buffer[5];
			u16 offset = buffer[6] << 8 | buffer[7];
			u32 length = buffer[8] << 24 | buffer[9] << 16 | buffer[10] << 8 | buffer[11];

			if (length > FAT_MAX_WILL_ALLOC) syscall_panic("FAT: request asked for more than we'd like to alloc");
			if (length > buffer_len) {
				delete [] buffer;
				buffer = new char[length];
				buffer_len = length;
			}

			partition_read_data(partition_id, sector, offset, length, buffer);

			syscall_puts("FAT: reply length "); syscall_putl(length, 16); syscall_putc('\n');
			syscall_sendQueue(info.from, info.id, buffer, length);
		} else {
			syscall_panic("FAT: confused");
		}
	}

	syscall_panic("FAT: went off the edge");
	return 1;
}

bool init() {
	partition_read_data(0, 0, 0, sizeof(fat_boot_record_t), reinterpret_cast<char *>(&boot_record));

	fat32esque = 0xff;
	if ((boot_record.total_sectors_small > 0) && (boot_record.ebr.signature == 0x28 || boot_record.ebr.signature == 0x29))
		fat32esque = 0;
	else if ((boot_record.total_sectors_large > 0) && (boot_record.fat32_ebr.fat_record.signature == 0x28 || boot_record.fat32_ebr.fat_record.signature == 0x29))
		fat32esque = 1;
	
	if (fat_type == 0xff) {
		syscall_puts("FAT: we don't understand this sort of FAT.\n");
		return false;
	}

	if (!fat32esque) {
		root_dir_sectors = ((boot_record.directory_entries * 32) + (boot_record.bytes_per_sector - 1)) / boot_record.bytes_per_sector;
		fat_sectors = boot_record.sectors_per_fat;
		first_data_sector = boot_record.reserved_sectors + fat_sectors * boot_record.fats;
		first_fat_sector = boot_record.reserved_sectors;
		data_sectors = boot_record.total_sectors_small - (boot_record.reserved_sectors + (boot_record.fats * boot_record.sectors_per_fat) + root_dir_sectors);
		total_clusters = data_sectors / boot_record.sectors_per_cluster;

		root_cluster = first_data_sector;

		if (total_clusters < 4095)
			fat_type = 12;
		else if (total_clusters < 65525)
			fat_type = 16;
		else if (total_clusters == 0)
			syscall_panic("FAT: we think it's not FAT32 yet the cluster count is 0.\n");
		else
			syscall_panic("FAT: we think it's not FAT32 yet the cluster count is 65525+.\n");
	} else {
		/* FAT32 */
		fat_sectors = (boot_record.fat32_ebr.fat_size + (boot_record.bytes_per_sector - 1)) / boot_record.bytes_per_sector;
		first_data_sector = boot_record.reserved_sectors + fat_sectors * boot_record.fats;
		first_fat_sector = boot_record.reserved_sectors;

		root_cluster = boot_record.fat32_ebr.root_directory_cluster;
		fat_type = 32;
	}

	#ifdef SHOW_FAT_INFORMATION
		syscall_puts("FAT: No. of FATs: "); syscall_putl(boot_record.fats, 16); syscall_putc('\n');
		syscall_puts("FAT: Sectors per FAT: "); syscall_putl(fat_sectors, 16); syscall_putc('\n');
		syscall_puts("FAT: Bytes per sector: "); syscall_putl(boot_record.bytes_per_sector, 16); syscall_putc('\n');
		syscall_puts("FAT: Sectors per cluster: "); syscall_putl(boot_record.sectors_per_cluster, 16); syscall_putc('\n');
		syscall_puts("FAT: First data sector: "); syscall_putl(first_data_sector, 16); syscall_putc('\n');
		syscall_puts("FAT: First FAT sector: "); syscall_putl(first_fat_sector, 16); syscall_putc('\n');

		if (!fat32esque) {
			syscall_puts("FAT: Total data sectors: "); syscall_putl(data_sectors, 16); syscall_putc('\n');
			syscall_puts("FAT: Total clusters: "); syscall_putl(total_clusters, 16); syscall_putc('\n');
			syscall_puts("FAT: Root dir sectors: "); syscall_putl(root_dir_sectors, 16); syscall_putc('\n');
		}

		syscall_puts("FAT: Root cluster: "); syscall_putl(root_cluster, 16); syscall_putc('\n');
	#endif

	fat_data = new u8[boot_record.sectors_per_cluster * boot_record.bytes_per_sector];

	partition_read_data(0, first_fat_sector, 0, boot_record.sectors_per_cluster * boot_record.bytes_per_sector, reinterpret_cast<char *>(fat_data));

	return true;
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

	u32 msg_id = syscall_sendQueue(mbr, 0, req, 12);

	struct queue_item_info *info = syscall_probeQueueFor(msg_id);
	if (info->data_len != length) syscall_panic("FAT: MBR read not expected number of bytes back?");

	syscall_readQueue(info, buffer, 0, info->data_len);
	syscall_shiftQueue(info);
}
