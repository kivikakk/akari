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

unsigned long write_data_fat(fs_node_t *node, unsigned long offset, unsigned long size, unsigned char *buffer)
{
	panic("woahh, tiger\n");
}

// TODO
void open_fat(fs_node_t *node) { }
void close_fat(fs_node_t *node) { }

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

