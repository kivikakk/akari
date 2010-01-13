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

typedef struct fat_extended_boot_record
{
	unsigned char drive_number;
	unsigned char nt_flags;
	unsigned char signature;	/* 0x28 or 0x29? */
	unsigned long serial_number;
	char volume_label[11];	/* space padded */
	char system_id[8];	/* space padded */

} __attribute__((__packed__)) fat_extended_boot_record_t;

typedef struct fat32_extended_boot_record
{
	unsigned long fat_size;
	unsigned short flags;
	union {
		unsigned short fat_version;
		struct {
			unsigned char minor;
			unsigned char major;
		} fat_version_comp;
	};
	unsigned long root_directory_cluster;
	unsigned short fsinfo_cluster;
	unsigned short backup_boot_cluster;
	unsigned char reserved[12];	/* set to NUL on format */
	fat_extended_boot_record_t fat_record;
} __attribute__((__packed__)) fat32_extended_boot_record_t;

typedef struct fat_boot_record
{
	unsigned char code_jump[3];
	char oem_identifier[8];	/* NUL padded */
	unsigned short bytes_per_sector;
	unsigned char sectors_per_cluster;
	unsigned short reserved_sectors;
	unsigned char fats;
	unsigned short directory_entries;
	unsigned short total_sectors_small;
	unsigned char media_descriptor;
	unsigned short sectors_per_fat;
	unsigned short sectors_per_track;
	unsigned short media_sides;
	unsigned long hidden_sectors;
	unsigned long total_sectors_large;

	union {
		fat_extended_boot_record_t ebr;
		fat32_extended_boot_record_t fat32_ebr;
	};
} __attribute__((__packed__)) fat_boot_record_t;

#define FAT_READ_ONLY	0x01
#define FAT_HIDDEN	0x02
#define FAT_SYSTEM	0x04
#define FAT_VOLUME_ID	0x08
#define FAT_DIRECTORY	0x10
#define FAT_ARCHIVE	0x20

typedef struct fat_dirent
{
	char filename[11];
	unsigned char attributes;
	unsigned char nt_reserved;
	unsigned char ctime;	/* 10ths of second */
	unsigned ctime_hours : 5;
	unsigned ctime_minutes : 6;
	unsigned ctime_seconds : 5;
	unsigned cdate_year : 7;
	unsigned cdate_month : 4;
	unsigned cdate_day : 5;
	unsigned adate_year : 7;
	unsigned adate_month : 4;
	unsigned adate_day : 5;
	unsigned short first_cluster_high;
	unsigned mtime_hours : 5;
	unsigned mtime_minutes : 6;
	unsigned mtime_seconds : 5;
	unsigned mdate_year : 7;
	unsigned mdate_month : 4;
	unsigned mdate_day : 5;
	unsigned short first_cluster_low;
	unsigned long size;
} __attribute__((__packed__)) fat_dirent_t;

void FATProcess() {
	if (!syscall_registerName("system.io.fat"))
		syscall_panic("could not register system.io.fat");

	syscall_puts("FAT driver entering loop\n");
#define FAT_BUFFER 10240
	char buffer[FAT_BUFFER];

	while (true) {
		struct queue_item_info info = *syscall_probeQueue();
		// We assign (copy) this so we don't lose it after shiftQueue().

		u32 len = info.data_len;
		if (len > FAT_BUFFER) syscall_panic("FAT buffer overflow");
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

			if (length > FAT_BUFFER) syscall_panic("Static buffer in FAT too small for req'd amount");

			partition_read_data(partition_id, sector, offset, length, buffer);

			syscall_sendQueue(info.from, info.id, buffer, length);
		} else {
			syscall_panic("FAT driver confused");
		}
	}
}


void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, char *buffer)
{
	// This is being REALLY wasteful by re-requesting the FAT EVERY REQUEST,
	// but what can you do?
	master_boot_record_t hdd_mbr;
	ata_read_data(0, 0, 512, reinterpret_cast<char *>(&hdd_mbr));

	if (hdd_mbr.signature != 0xAA55) syscall_panic("Invalid MBR!\n");
	
	u32 new_sector = hdd_mbr.partitions[partition_id].begin_disk_sector + sector;

	ata_read_data(new_sector, offset, length, buffer);
}

void parition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, char *buffer) {
	// Request to MBR driver: u8 0x0 ('read'), u8 parition_id, u32 sector, u16 offset, u32 length
	// Total: 12 bytes
	
	char req[] = {
		0,
		partition_id,
		(new_sector >> 24) & 0xFF, (new_sector >> 16) & 0xFF, (new_sector >> 8) & 0xFF, new_sector & 0xFF,
		(offset >> 8) & 0xFF, offset & 0xFF,
		(length >> 24) & 0xFF, (length >> 16) & 0xFF, (length >> 8) & 0xFF, length & 0xFF
	};

	// u32 id =
	syscall_sendQueue(syscall_processIdByName("system.io.mbr"), 0, req, 12);;

	// TODO XXX: need to listen for replies to a certain message, not just the next one.

	struct queue_item_info *info = syscall_probeQueue();
	if (info->data_len != length) syscall_panic("FAT's MBR read: not expected number of bytes back?");

	syscall_readQueue(buffer, 0, info->data_len);
	syscall_shiftQueue();
}
