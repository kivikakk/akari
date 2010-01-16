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

#ifndef MAIN_HPP
#define MAIN_HPP

#define FAT_MAX_WILL_ALLOC 0x80000

typedef struct fat_extended_boot_record
{
	u8 drive_number;
	u8 nt_flags;
	u8 signature;	/* 0x28 or 0x29? */
	u32 serial_number;
	char volume_label[11];	/* space padded */
	char system_id[8];	/* space padded */

} __attribute__((__packed__)) fat_extended_boot_record_t;

typedef struct fat32_extended_boot_record
{
	u32 fat_size;
	u16 flags;
	union {
		u16 fat_version;
		struct {
			u8 minor;
			u8 major;
		} fat_version_comp;
	};
	u32 root_directory_cluster;
	u16 fsinfo_cluster;
	u16 backup_boot_cluster;
	u8 reserved[12];	/* set to NUL on format */
	fat_extended_boot_record_t fat_record;
} __attribute__((__packed__)) fat32_extended_boot_record_t;

typedef struct fat_boot_record
{
	u8 code_jump[3];
	char oem_identifier[8];	/* NUL padded */
	u16 bytes_per_sector;
	u8 sectors_per_cluster;
	u16 reserved_sectors;
	u8 fats;
	u16 directory_entries;
	u16 total_sectors_small;
	u8 media_descriptor;
	u16 sectors_per_fat;
	u16 sectors_per_track;
	u16 media_sides;
	u32 hidden_sectors;
	u32 total_sectors_large;

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
	u8 filename[11];
	u8 attributes;
	u8 nt_reserved;
	u8 ctime;	/* 10ths of second */
	unsigned ctime_hours : 5;
	unsigned ctime_minutes : 6;
	unsigned ctime_seconds : 5;
	unsigned cdate_year : 7;
	unsigned cdate_month : 4;
	unsigned cdate_day : 5;
	unsigned adate_year : 7;
	unsigned adate_month : 4;
	unsigned adate_day : 5;
	u16 first_cluster_high;
	unsigned mtime_hours : 5;
	unsigned mtime_minutes : 6;
	unsigned mtime_seconds : 5;
	unsigned mdate_year : 7;
	unsigned mdate_month : 4;
	unsigned mdate_day : 5;
	u16 first_cluster_low;
	u32 size;
} __attribute__((__packed__)) fat_dirent_t;

#endif

