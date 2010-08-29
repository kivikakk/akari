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
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>
#include <debug.hpp>
#include <map>

#include "main.hpp"
#include "../VFS/VFSProto.hpp"
#include "../ATA/ATAProto.hpp"

bool init();
u8 *fat_read_fat_cluster(u32 cluster);
void fat_read_cluster(u32 cluster, u8 *buffer);
void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, u8 *buffer);
u32 fat_read_data(u32 inode, u32 offset, u32 length, u8 *buffer);
u32 fat_entry_for(u32 for_cluster);
VFSDirent *fat_readdir(u32 inode, u32 index);
VFSNode *fat_finddir(u32 inode, const char *name);
static u8 *get_filename(const fat_dirent_t *fd);

pid_t ata = 0, vfs = 0;
u32 vfs_driver_no = 0;

static fat_boot_record_t boot_record;

static u32 first_data_sector, first_fat_sector, fat_sectors;
static u8 fat_type, fat32esque;
static u32 root_cluster;

static std::map<u32, u8 *> fat_clusters;

extern "C" int main() {
	ata = processIdByNameBlock("system.io.ata");

	// Initialize our important stuff
	if (!init()) {
		printf("FAT: calling it quits in init");
		return 1;
	}

	// Register our name
	if (!registerName("system.io.fs.fat"))
		panic("FAT: could not register system.io.fs.fat");

	vfs = processIdByNameBlock("system.io.vfs");

	// Register with VFS
	{
		const char *driver_name = "fat";
		u32 cmd_len = sizeof(VFSOpRegisterDriver) + strlen(driver_name) + 1 - 1;	// trailing NUL, less the [1] in the struct
		VFSOpRegisterDriver *register_driver_op = static_cast<VFSOpRegisterDriver *>(malloc(cmd_len));
		register_driver_op->cmd = VFS_OP_REGISTER_DRIVER;
		strcpy(register_driver_op->name, "fat");

		u32 msg_id = sendQueue(vfs, 0, reinterpret_cast<u8 *>(register_driver_op), cmd_len);
		free(register_driver_op);

		struct queue_item_info *info = probeQueueFor(msg_id);
		if (info->data_len != sizeof(VFSReplyRegisterDriver)) panic("FAT: VFS gave weird reply to attempt to register");

		VFSReplyRegisterDriver reply;
		readQueue(info, reinterpret_cast<u8 *>(&reply), 0, info->data_len);
		shiftQueue(info);

		if (!reply.success) {
			printf("FAT: failed to register; VFS said 0x%x\n", reply.driver);
			panic("FAT: failed to register with VFS");
		}

		vfs_driver_no = reply.driver;
	}
	
	// Mount ourselves as root
	{
		VFSOpMountRoot mount_root_op = { VFS_OP_MOUNT_ROOT, vfs_driver_no, 0 };
		
		u32 msg_id = sendQueue(vfs, 0, reinterpret_cast<u8 *>(&mount_root_op), sizeof(mount_root_op));
		struct queue_item_info *info = probeQueueFor(msg_id);
		u8 *reply = grabQueue(info);
		if (info->data_len != 1 || reply[0] != 1) panic("FAT: couldn't mount self as root");
		shiftQueue(info);
		delete [] reply;

	}

	// All done.
	printf("FAT: started\n");

	while (true) {
		struct queue_item_info info = *probeQueue();
		u8 *request = grabQueue(&info);
		shiftQueue(&info);

		if (request[0] == VFS_OP_READ) {
			VFSOpRead *op = reinterpret_cast<VFSOpRead *>(request);

			u8 *buffer = new u8[op->length];
			u32 bytes_read = fat_read_data(op->inode, op->offset, op->length, buffer);
			sendQueue(info.from, info.id, buffer, bytes_read);
			delete [] buffer;
		} else if (request[0] == VFS_OP_READDIR) {
			VFSOpReaddir *op = reinterpret_cast<VFSOpReaddir *>(request);

			VFSDirent *dirent = fat_readdir(op->inode, op->index);
			if (!dirent) {
				sendQueue(info.from, info.id, 0, 0);
			} else {
				sendQueue(info.from, info.id, reinterpret_cast<u8 *>(dirent), sizeof(VFSDirent));
				delete dirent;
			}
		} else if (request[0] == VFS_OP_FINDDIR) {
			VFSOpFinddir *op = reinterpret_cast<VFSOpFinddir *>(request);

			VFSNode *node = fat_finddir(op->inode, op->name);
			if (!node) {
				sendQueue(info.from, info.id, 0, 0);
			} else {
				sendQueue(info.from, info.id, reinterpret_cast<u8 *>(node), sizeof(VFSNode));
				delete node;
			}
		} else {
			panic("FAT: confused");
		}

		delete [] request;
	}
}

bool init() {
	partition_read_data(0, 0, 0, sizeof(fat_boot_record_t), reinterpret_cast<u8 *>(&boot_record));

	fat32esque = 0xff;
	if ((boot_record.total_sectors_small > 0) && (boot_record.ebr.signature == 0x28 || boot_record.ebr.signature == 0x29))
		fat32esque = 0;
	else if ((boot_record.total_sectors_large > 0) && (boot_record.fat32_ebr.fat_record.signature == 0x28 || boot_record.fat32_ebr.fat_record.signature == 0x29))
		fat32esque = 1;
	
	if (fat_type == 0xff) {
		printf("FAT: we don't understand this sort of FAT.\n");
		return false;
	}

	if (!fat32esque) {
		panic("no !FAT32");
		// root_dir_sectors = ((boot_record.directory_entries * 32) + (boot_record.bytes_per_sector - 1)) / boot_record.bytes_per_sector;
		fat_sectors = boot_record.sectors_per_fat;
		first_data_sector = boot_record.reserved_sectors + fat_sectors * boot_record.fats;
		first_fat_sector = boot_record.reserved_sectors;
		// data_sectors = boot_record.total_sectors_small - (boot_record.reserved_sectors + (boot_record.fats * boot_record.sectors_per_fat) + root_dir_sectors);
		// total_clusters = data_sectors / boot_record.sectors_per_cluster;

		root_cluster = first_data_sector;

		// if (total_clusters < 4095)
			// fat_type = 12;
		// else if (total_clusters < 65525)
			// fat_type = 16;
		// else if (total_clusters == 0)
			// panic("FAT: we think it's not FAT32 yet the cluster count is 0.\n");
		// else
			// panic("FAT: we think it's not FAT32 yet the cluster count is 65525+.\n");
	} else {
		/* FAT32 */
		fat_sectors = boot_record.fat32_ebr.fat_size;
		first_data_sector = boot_record.reserved_sectors + fat_sectors * boot_record.fats;
		first_fat_sector = boot_record.reserved_sectors;

		root_cluster = boot_record.fat32_ebr.root_directory_cluster;
		fat_type = 32;
	}

	#ifdef SHOW_FAT_INFORMATION
		printf("FAT: No. of FATs: 0x%x\n", boot_record.fats);
		printf("FAT: Sectors per FAT: 0x%x\n", fat_sectors);
		printf("FAT: Bytes per sector: 0x%x\n", boot_record.bytes_per_sector);
		printf("FAT: Sectors per cluster: 0x%x\n", boot_record.sectors_per_cluster);
		printf("FAT: First data sector: 0x%x\n", first_data_sector);
		printf("FAT: First FAT sector: 0x%x\n", first_fat_sector);

		if (!fat32esque) {
			// printf("FAT: Total data sectors: 0x%x\n", data_sectors);
			// printf("FAT: Total clusters: 0x%x\n", total_clusters);
			// printf("FAT: Root dir sectors: 0x%x\n", root_dir_sectors);
		}

		printf("FAT: Root cluster: 0x%x\n", root_cluster);
	#endif

	fat_read_fat_cluster(0);

	return true;
}

u8 *fat_read_fat_cluster(u32 cluster) {
	std::map<u32, u8 *>::iterator it = fat_clusters.find(cluster);
	if (it != fat_clusters.end())
		return it->second;

	u8 *cluster_data = new u8[boot_record.sectors_per_cluster * boot_record.bytes_per_sector];
	partition_read_data(0, first_fat_sector + cluster * boot_record.sectors_per_cluster, 0, boot_record.bytes_per_sector * boot_record.sectors_per_cluster, cluster_data);

	fat_clusters[cluster] = cluster_data;

	return cluster_data;
}

void fat_read_cluster(u32 cluster, u8 *buffer) {
	if (!fat32esque) {
		// partition_read_data(0, root_cluster + root_dir_sectors + (cluster - 2) * boot_record.sectors_per_cluster, 0, boot_record.bytes_per_sector * boot_record.sectors_per_cluster, buffer);
		panic("FAT not impl for !FAT32");
	} else {
		partition_read_data(0, first_data_sector + (cluster - 2) * boot_record.sectors_per_cluster, 0, boot_record.bytes_per_sector * boot_record.sectors_per_cluster, buffer);
	}
}

void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, u8 *buffer) {
	// Request to ATA driver: u8 0x0 ('read'), u8 parition_id, u32 sector, u16 offset, u32 length
	// Total: 12 bytes
	
	ATAOpMBRRead op = {
		ATA_OP_MBR_READ,
		partition_id,
		sector,
		offset,
		length
	};

	u32 msg_id = sendQueue(ata, 0, reinterpret_cast<u8 *>(&op), sizeof(ATAOpMBRRead));

	struct queue_item_info *info = probeQueueFor(msg_id);
	if (info->data_len != length) panic("FAT: ATA read not expected number of bytes back?");

	readQueue(info, buffer, 0, info->data_len);
	shiftQueue(info);
}


u32 fat_read_data(u32 inode, u32 offset, u32 length, u8 *buffer) {
	u32 current_cluster = inode;
	static u8 *scratch = 0;
	static u32 scratch_len = 0;

	if (scratch && scratch_len < boot_record.bytes_per_sector * boot_record.sectors_per_cluster) {
		delete [] scratch;
		scratch = 0;
		scratch_len = 0;
	}

	if (!scratch) {
		scratch = new u8[
			(scratch_len =
			 boot_record.bytes_per_sector * boot_record.sectors_per_cluster)];
	}

	while (offset > boot_record.bytes_per_sector * boot_record.sectors_per_cluster) {
		u32 fat_entry = fat_entry_for(current_cluster);
		if (fat_entry == 0) panic("FAT: lead to a free cluster!");
		else if (fat_entry == 1) panic("FAT: lead to a reserved cluster!");
		else if (fat_entry >= 0x0FFFFFF0 && fat_entry <= 0x0FFFFFF7) panic("FAT: lead to an end-reserved cluster!");
		else if (fat_entry >= 0x0FFFFFF8) {
			// EOF
			return 0;
		} else {
			current_cluster = fat_entry;
			offset -= boot_record.bytes_per_sector * boot_record.sectors_per_cluster;
		}
	}

	u32 copied = 0;
	while (length > 0) {
		fat_read_cluster(current_cluster, scratch);
		u16 copy_len = min(length, boot_record.bytes_per_sector * boot_record.sectors_per_cluster - offset);
		memcpy(buffer, scratch + offset, copy_len);
		offset = 0;

		buffer += copy_len; length -= copy_len; copied += copy_len;

		if (length) {
			u32 fat_entry = fat_entry_for(current_cluster);

			if (fat32esque) {
				if (fat_entry == 0) panic("FAT: lead to a free cluster!");
				else if (fat_entry == 1) panic("FAT: lead to a reserved cluster!");
				else if (fat_entry >= 0x0FFFFFF0 && fat_entry <= 0x0FFFFFF7) panic("FAT: lead to an end-reserved cluster!");
				else if (fat_entry >= 0x0FFFFFF8) {
					// looks like end-of-file.
					return copied;
				} else {
					current_cluster = fat_entry;
				}
			} else {
				panic("not !FAT32");
				if (fat_entry == 0) panic("FAT: lead to a free cluster!");
				else if (fat_entry == 1) panic("FAT: lead to a reserved cluster!");
				else if (fat_entry >= 0xFFF0 && fat_entry <= 0xFFF7) panic("FAT: lead to an end-reserved cluster!");
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

u32 fat_entry_for(u32 for_cluster) {
	// follow cluster
	u32 bytes_per_cluster = boot_record.sectors_per_cluster * boot_record.bytes_per_sector;
	u32 bytes_per_entry = fat32esque ? sizeof(u32) : sizeof(u16);
	u32 entries_per_cluster = bytes_per_cluster / bytes_per_entry;
	u32 relative_cluster = for_cluster / entries_per_cluster;
	u32 cluster_index = for_cluster - relative_cluster * entries_per_cluster;

	void *cluster = fat_read_fat_cluster(relative_cluster);

	if (fat32esque) {
		u32 fat_entry = reinterpret_cast<u32 *>(cluster)[cluster_index];
		fat_entry &= 0x0FFFFFFF;
		return fat_entry;
	} else {
		panic("no !FAT32");
		u16 fat_entry = reinterpret_cast<u16 *>(cluster)[cluster_index];
		return fat_entry;
	}
}

static u32 inline cluster_for_inode(u32 inode) {
	return inode == 0 ? root_cluster : inode;
}

VFSDirent *fat_readdir(u32 inode, u32 index) {
	static u8 *cluster_buffer = 0;
	if (!cluster_buffer)
		cluster_buffer = new u8[boot_record.sectors_per_cluster * boot_record.bytes_per_sector];

	u32 current_cluster = cluster_for_inode(inode);
	u32 current = 0;

	if (inode == 0) {
		// Root dir. Add mythical . (0) and .. (1) entries if
		// they ask, otherwise start with the current index adjusted
		// so it looks like they exist.
		
		if (index <= 1) {
			VFSDirent *dirent = new VFSDirent;
			strcpy(dirent->name, index == 0 ? "." : "..");
			dirent->inode = 0;
			return dirent;
		}

		current = 2;
	}

	while (true) {
		fat_read_cluster(current_cluster, cluster_buffer);
		
		fat_dirent_t *fd = reinterpret_cast<fat_dirent_t *>(cluster_buffer);

		for (u32 position = 0; position < (4096 / 32) * 1; ++position, ++fd) {
			if (fd->filename[0] == 0)
				break;
			else if (fd->filename[0] == 0xe5)
				continue;
			else if (fd->filename[11] == 0x0f) {
				continue;	// TODO LFN
			}
			else if (fd->attributes & FAT_VOLUME_ID) {
				continue;	// fd->filename is vol ID
			}

			// dir or file now!
			else if (current++ == index) {
				// bingo!
				VFSDirent *dirent = new VFSDirent;

				u8 *filename = get_filename(fd);
				strcpy(dirent->name, reinterpret_cast<char *>(filename));

				dirent->inode = (fd->first_cluster_high << 16) | fd->first_cluster_low;

				return dirent;
			}
		}

		// Didn't find it here.
		current_cluster = fat_entry_for(current_cluster);
		if (current_cluster == 0) panic("FAT: lead to a free cluster!");
		else if (current_cluster == 1) panic("FAT: lead to a reserved cluster!");
		else if (current_cluster >= 0x0FFFFFF0 && current_cluster <= 0x0FFFFFF7) panic("FAT: lead to an end-reserved cluster!");
		else if (current_cluster >= 0x0FFFFFF8) {
			// EOF
			return 0;
		} 
	}

	return 0;
}

VFSNode *fat_finddir(u32 inode, const char *name) {
	static u8 *cluster_buffer = 0;
	if (!cluster_buffer)
		cluster_buffer = new u8[boot_record.sectors_per_cluster * boot_record.bytes_per_sector];

	u32 current_cluster = cluster_for_inode(inode);

	// Root . and .. entries
	if (inode == 0 && (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)) {
		VFSNode *node = new VFSNode;
		strcpy(node->name, name);
		node->flags = VFS_DIRECTORY;
		node->inode = 0;
		node->length = node->impl = 0;
		node->driver = vfs_driver_no;

		return node;
	}

	while (true) {
		fat_read_cluster(current_cluster, cluster_buffer);

		fat_dirent_t *fd = reinterpret_cast<fat_dirent_t *>(cluster_buffer);

		for (u32 position = 0; position < (4096 / 32) * 1; ++position, ++fd) {
			if (fd->filename[0] == 0)
				break;
			else if (fd->filename[0] == 0xe5)
				continue;
			else if (fd->filename[11] == 0x0f)
				continue;	// TODO LFN
			else if (fd->attributes & FAT_VOLUME_ID)
				continue;

			u8 *filename = get_filename(fd);
			if (stricmp(reinterpret_cast<char *>(filename), name) == 0) {
				VFSNode *node = new VFSNode;
				strcpy(node->name, name);
				node->flags = (fd->attributes & FAT_DIRECTORY) ? VFS_DIRECTORY : VFS_FILE;
				node->inode = (fd->first_cluster_high << 16) | fd->first_cluster_low;
				node->length = fd->size;
				node->impl = 0;
				node->driver = vfs_driver_no;

				// TODO: set node fs to FAT
				return node;
			}
		}

		current_cluster = fat_entry_for(current_cluster);
		if (current_cluster == 0) panic("FAT: lead to a free cluster!");
		else if (current_cluster == 1) panic("FAT: lead to a reserved cluster!");
		else if (current_cluster >= 0x0FFFFFF0 && current_cluster <= 0x0FFFFFF7) panic("FAT: lead to an end-reserved cluster!");
		else if (current_cluster >= 0x0FFFFFF8) {
			// EOF
			return 0;
		} 
	}

	return 0;
}

static u8 *get_filename(const fat_dirent_t *fd) {
	static u8 filename_return[13];

	u8 *write = filename_return;
	const u8 *read = fd->filename;
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
