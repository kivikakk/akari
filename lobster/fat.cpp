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

#include <fat.hpp>
#include <mbr.hpp>
#include <screen.hpp>
#include <string.hpp>
#include <memory.hpp>
#include <panic.hpp>

static char *get_filename(const fat_dirent_t *fd);

static fat_boot_record_t boot_record;
static unsigned char *fat_data;

static unsigned long root_dir_sectors = 0xdeadbeef, first_data_sector = 0xdeadbeef, first_fat_sector = 0xdeadbeef, data_sectors = 0xdeadbeef, total_clusters = 0xdeadbeef, fat_sectors = 0xdeadbeef;
static unsigned char fat_type, fat32esque;
static unsigned long root_cluster = 0xdeadbeef;

void installfat(void)
{
	/* We'll just try one. */
	partition_read_data(0, 0, 0, sizeof(fat_boot_record_t), (unsigned char *)&boot_record);

	fat32esque = 0xff;
	if ((boot_record.total_sectors_small > 0) && (boot_record.ebr.signature == 0x28 || boot_record.ebr.signature == 0x29))
		fat32esque = 0;
	else if ((boot_record.total_sectors_large > 0) && (boot_record.fat32_ebr.fat_record.signature == 0x28 || boot_record.fat32_ebr.fat_record.signature == 0x29))
		fat32esque = 1;
	
	if (fat_type == 0xff) {
		puts("We don't understand this sort of FAT.\n");
		return;
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
			panic("We think it's not FAT32 yet the cluster count is 0.\n");
		else
			panic("We think it's not FAT32 yet the cluster count is 65525+.\n");
	} else {
		/* FAT32 */
		fat_sectors = (boot_record.fat32_ebr.fat_size + (boot_record.bytes_per_sector - 1)) / boot_record.bytes_per_sector;
		first_data_sector = boot_record.reserved_sectors + fat_sectors * boot_record.fats;
		first_fat_sector = boot_record.reserved_sectors;

		root_cluster = boot_record.fat32_ebr.root_directory_cluster;
		fat_type = 32;
	}

	#ifdef SHOW_FAT_INFORMATION
		puts("No. of FATs: "); puthexlong(boot_record.fats); locatenl();
		puts("Sectors per FAT: "); puthexlong(fat_sectors); locatenl();
		puts("Bytes per sector: "); puthexlong(boot_record.bytes_per_sector); locatenl();
		puts("Sectors per cluster: "); puthexlong(boot_record.sectors_per_cluster); locatenl();
		puts("First data sector: "); puthexlong(first_data_sector); locatenl();
		puts("First FAT sector: "); puthexlong(first_fat_sector); locatenl();

		if (!fat32esque) {
			puts("Total data sectors: "); puthexlong(data_sectors); locatenl();
			puts("Total clusters: "); puthexlong(total_clusters); locatenl();
			puts("Root dir sectors: "); puthexlong(root_dir_sectors); locatenl();
		}

		puts("Root cluster: "); puthexlong(root_cluster); locatenl();
	#endif

	fat_data = (unsigned char *)kmalloc(boot_record.sectors_per_cluster * boot_record.bytes_per_sector);

	partition_read_data(0, first_fat_sector, 0, boot_record.sectors_per_cluster * boot_record.bytes_per_sector, fat_data);
}

unsigned long read_data_fat(fs_node_t *node, unsigned long offset, unsigned long size, unsigned char *buffer)
{ 
	unsigned long current_cluster = node->inode, fat_entry;
	unsigned short copy_len;
	unsigned long copied = 0;
	unsigned char scratch[2048];

	// TODO: follow clusters while offset>=2048

	// cap the read to the actual bounds of the file
	if (size + offset > node->length)
		size = node->length - offset;	// if it goes negative, the loop will just never go through

	while (size > 0) {
		fat_read_cluster(current_cluster, scratch);
		copy_len = (size > (2048 - offset) ? (2048 - offset) : size);
		memcpy(buffer, scratch + offset, copy_len);

		buffer += copy_len; size -= copy_len; copied += copy_len;
		offset = 0;

		// follow cluster if we have to
		if (size) {
			if (fat32esque) {
				fat_entry = ((unsigned long *)fat_data)[current_cluster];
			} else {
				fat_entry = ((unsigned short *)fat_data)[current_cluster];
				if (fat_entry == 0)
					panic("Lead to a free cluster!\n");
				else if (fat_entry == 1)
					panic("Lead to a reserved cluster!\n");
				else if (fat_entry >= 0xfff0 && fat_entry <= 0xfff7)
					panic("Lead to an end-reserved cluster!\n");
				else if (fat_entry >= 0xfff8)
					return copied;
				else
					current_cluster = fat_entry;
			}
		}
	}


	return copied;
}

unsigned long write_data_fat(fs_node_t *node, unsigned long offset, unsigned long size, unsigned char *buffer)
{
	panic("woahh, tiger\n");
}

// TODO
void open_fat(fs_node_t *node) { }
void close_fat(fs_node_t *node) { }

dirent_t *readdir_fat(fs_node_t *node, unsigned long index)
{
	unsigned char *cluster = (unsigned char *)kmalloc(512 * root_dir_sectors);

	if (node->inode == 0) {
		// root inode
		// <-- note this next line completely disagrees with how we do FAT32
		partition_read_data(0, root_cluster, 0, 512 * root_dir_sectors, cluster);
		
		fat_dirent_t *fd = (fat_dirent_t *)cluster;
		unsigned long current = 0, position;
		for (position = 0; position < (512 / 32) * root_dir_sectors; ++position, ++fd) {
			if (fd->filename[0] == 0)
				break;
			else if(fd->filename[0] == (signed char)0xe5)
				continue;
			else if (fd->filename[11] == 0x0f)
				continue;	// TODO LFN
			else if (fd->attributes & FAT_VOLUME_ID)
				continue;	// fd->filename is VOL ID

			// it must be a directory or file by this stage
			else if (current++ == index) {
				// we found it, baby.
				dirent_t *my_dirent = (dirent_t *)kmalloc(sizeof(dirent_t));
				char *filename = get_filename(fd);
				strcpy(my_dirent->name, filename);
				kfree(filename);
				my_dirent->ino = (fd->first_cluster_high << 16) | fd->first_cluster_low;
				kfree(cluster);
				return my_dirent;
			}
		}

		kfree(cluster);
		return 0;		// 404 Not Found
	} 

	kfree(cluster);
	panic("whoops.\n");
}

fs_node_t *finddir_fat(fs_node_t *node, const char *name)
{
	// unsigned char cluster[2048 * 32];
	unsigned char *cluster = (unsigned char *)kmalloc(512 * root_dir_sectors);

	if (node->inode == 0) {
		// root inode
		// <-- note this next line completely disagrees with how we do FAT32
		partition_read_data(0, root_cluster, 0, 512 * root_dir_sectors, cluster);
		
		fat_dirent_t *fd = (fat_dirent_t *)cluster;
		unsigned long position;
		for (position = 0; position < (512 / 32) * root_dir_sectors; ++position, ++fd) {
			if (fd->filename[0] == 0)
				break;
			else if(fd->filename[0] == (signed char)0xe5)
				continue;
			else if (fd->filename[11] == 0x0f)
				continue;	// TODO LFN
			else if (fd->attributes & FAT_VOLUME_ID)
				continue;	// fd->filename is VOL ID

			char *filename = get_filename(fd);
			if (stricmp(filename, name) == 0) {
				kfree(filename);

				// create an fs_node_t for this fat_dirent_t.
				fs_node_t *my_node = (fs_node_t *)kmalloc(sizeof(fs_node_t));
				strcpy(my_node->name, name);
				my_node->flags = 
					(fd->attributes & FAT_DIRECTORY) ? FS_DIRECTORY : FS_FILE;
				my_node->inode = (fd->first_cluster_high << 16) | fd->first_cluster_low;
				my_node->length = fd->size;
				my_node->impl = 0;

				my_node->read = read_data_fat; my_node->write = write_data_fat;
				my_node->open = open_fat; my_node->close = close_fat;
				my_node->readdir = 0; my_node->finddir = 0;
				my_node->ptr = 0;
				kfree(cluster);
				return my_node;
			}
			kfree(filename);
		}

		kfree(cluster);
		return 0;		// 404 Not Found
	} 
	kfree(cluster);

	panic("whoops.\n");
}

unsigned long fat_root_dir_length()
{
	if (fat32esque)
		/* XXX TODO: calculate cluster chain length */
		return 0xdeadbeef;
	else
		return root_dir_sectors;
}

void fat_read_cluster(unsigned long cluster, unsigned char *buffer)
{
	/* `absolute' cluster */
	partition_read_data(0, root_cluster + root_dir_sectors + (cluster - 2) * boot_record.sectors_per_cluster, 0, boot_record.bytes_per_sector * boot_record.sectors_per_cluster, buffer);
}

static char *get_filename(const fat_dirent_t *fd)
{
	char *filename_return = (char *)kmalloc(13);

	char *write = filename_return;
	const char *read = fd->filename;
	unsigned char read_past_tense = 0;
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
