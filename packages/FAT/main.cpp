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
void fat_read_cluster(u32 cluster, char *buffer);
void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, char *buffer);
u32 fat_read_data(u32 inode, u32 offset, u32 length, char *buffer);
VFSDirent *fat_readdir(u32 inode, u32 index);
VFSNode *fat_finddir(u32 inode, const char *name);
static char *get_filename(const fat_dirent_t *fd);

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
			u32 inode = buffer[1] << 24 | buffer[2] << 16 | buffer[3] << 8 | buffer[4],
				offset = buffer[5] << 24 | buffer[6] << 16 | buffer[7] << 8 | buffer[8],
				length = buffer[9] << 24 | buffer[10] << 16 | buffer[11] << 8 | buffer[12];

			if (length > FAT_MAX_WILL_ALLOC) syscall_panic("FAT: request asked for more than we'd like to alloc");
			if (length > buffer_len) {
				delete [] buffer;
				buffer = new char[length];
				buffer_len = length;
			}

			u32 bytes_read = fat_read_data(inode, offset, length, buffer);
			syscall_sendQueue(info.from, info.id, buffer, bytes_read);
		} else if (buffer[0] == VFS_OP_READDIR) {
			u32 inode = buffer[1] << 24 | buffer[2] << 16 | buffer[3] << 8 | buffer[4],
				index = buffer[5] << 24 | buffer[6] << 16 | buffer[7] << 8 | buffer[8];

			fat_readdir(inode, index);
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

void fat_read_cluster(u32 cluster, char *buffer) {
	partition_read_data(0, root_cluster + root_dir_sectors + (cluster - 2) * boot_record.sectors_per_cluster, 0, boot_record.bytes_per_sector * boot_record.sectors_per_cluster, buffer);
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


u32 fat_read_data(u32 inode, u32 offset, u32 length, char *buffer) {
	// TODO: follow clusters while offset >= 2048
	// TODO: figure out what the above TODO means
	
	u32 current_cluster = inode;
	static char scratch[2048];

	// XXX We don't know node->length!
	/*
	if (length + offset > node->length) {
		if (offset > node->length) return 0;
		length = node->length - offset;
	}
	*/

	u32 copied = 0;
	while (length > 0) {
		fat_read_cluster(current_cluster, scratch);
		u16 copy_len = min(length, 2048 - offset);
		syscall_memcpy(buffer, scratch + offset, copy_len);

		buffer += copy_len; length -= copy_len; copied += copy_len;

		if (length) {
			// follow cluster
			if (fat32esque) {
				// u32 fat_entry = reinterpret_cast<u32 *>(fat_data)[current_cluster];
			} else {
				u32 fat_entry = reinterpret_cast<u32 *>(fat_data)[current_cluster];
				if (fat_entry == 0) syscall_panic("FAT: lead to a free cluster!");
				else if (fat_entry == 1) syscall_panic("FAT: lead to a reserved cluster!");
				else if (fat_entry >= 0xFFF0 && fat_entry <= 0xFFF7) syscall_panic("FAT: lead to an end-reserved cluster!");
				else if (fat_entry >= 0xFFF8) {
					// looks like end-of-file.
					return copied;
				} else {
					current_cluster = fat_entry;
				}
			}
		}
	}

	return copied;
}

VFSDirent *fat_readdir(u32 inode, u32 index) {

	if (inode == 0) {
		// root
		char *cluster = new char[512 * root_dir_sectors];
		partition_read_data(0, root_cluster, 0, 512 * root_dir_sectors, cluster);
		
		fat_dirent_t *fd = reinterpret_cast<fat_dirent_t *>(cluster);

		u32 current = 0;
		for (u32 position = 0; position < (512 / 32) * root_dir_sectors; ++position, ++fd) {
			if (fd->filename[0] == 0)
				break;
			else if (fd->filename[0] == 0xe5)
				continue;
			else if (fd->filename[11] == 0x0f)
				continue;	// TODO LFN
			else if (fd->attributes & FAT_VOLUME_ID)
				continue;	// fd->filename is vol ID

			// dir or file now!
			else if (current++ == index) {
				// bingo!
				VFSDirent *dirent = new VFSDirent;

				char *filename = get_filename(fd);
				syscall_strcpy(dirent->name, filename);
				delete [] filename;

				dirent->inode = (fd->first_cluster_high << 16) | fd->first_cluster_low;

				delete [] cluster;
				return dirent;
			}
		}

		delete [] cluster;
		return 0;
	}

	syscall_panic("FAT: oops. can't readdir not-0 now.");
	return 0;
}

VFSNode *fat_finddir(u32 inode, const char *name) {
	if (inode == 0) {
		// root
		char *cluster = new char[512 * root_dir_sectors];
		partition_read_data(0, root_cluster, 0, 512 * root_dir_sectors, cluster);

		fat_dirent_t *fd = reinterpret_cast<fat_dirent_t *>(cluster);
		for (u32 position = 0; position < (512 / 32) * root_dir_sectors; ++position, ++fd) {
			if (fd->filename[0] == 0)
				break;
			else if (fd->filename[0] == 0xe5)
				continue;
			else if (fd->filename[11] == 0x0f)
				continue;	// TODO LFN
			else if (fd->attributes & FAT_VOLUME_ID)
				continue;

			char *filename = get_filename(fd);
			if (syscall_stricmp(filename, name) == 0) {
				delete [] filename;

				VFSNode *node = new VFSNode;
				syscall_strcpy(node->name, name);
				node->flags = (fd->attributes & FAT_DIRECTORY) ? VFS_DIRECTORY : VFS_FILE;
				node->inode = (fd->first_cluster_high << 16) | fd->first_cluster_low;
				node->length = fd->size;
				node->impl = 0;

				// TODO: set node fs to FAT
				delete [] cluster;
				return node;
			}
			delete [] filename;
		}

		delete [] cluster;
		return 0;
	}

	syscall_panic("FAT: oops. can't finddir not-0");
	return 0;
}

static char *get_filename(const fat_dirent_t *fd) {
	char *filename_return = new char[13];

	char *write = filename_return;
	const char *read = fd->filename;
	u8 read_past_tense = 0;
	while (read_past_tense < 8 && *read > 0x20) {
		read_past_tense++;
		*write++ = *read++;
	}
	read += (8 - read_past_tense);
	read_past_tense = 0;
	if (*read > 0x20) {
		*write++ = '.';
		while (read_past_tense < 3 && *read > 0x20) {
			read_past_tense++;
			*write++ = *read++;
		}
	}
	*write++ = 0;
	return filename_return;
}
