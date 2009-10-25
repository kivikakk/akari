#ifndef __FAT_HPP__
#define __FAT_HPP__

#include <vfs.hpp>
#include <cpp.hpp>

cextern unsigned long fat_read_fs(fs_node_t *node, unsigned long offset, unsigned long size, unsigned char *buffer);
cextern unsigned long fat_write_fs(fs_node_t *node, unsigned long offset, unsigned long size, unsigned char *buffer);
cextern void fat_open_fs(fs_node_t *node);
cextern void fat_close_fs(fs_node_t *node);
cextern dirent_t *fat_readdir_fs(fs_node_t *node, unsigned long index);
cextern fs_node_t *fat_finddir_fs(fs_node_t *node, char *name);

cextern void installfat(void);

cextern typedef struct fat_extended_boot_record
{
	unsigned char drive_number;
	unsigned char nt_flags;
	unsigned char signature;	/* 0x28 or 0x29? */
	unsigned long serial_number;
	char volume_label[11];	/* space padded */
	char system_id[8];	/* space padded */

} __attribute__((__packed__)) fat_extended_boot_record_t;

cextern typedef struct fat32_extended_boot_record
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

cextern typedef struct fat_boot_record
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

cextern typedef struct fat_dirent
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

cextern void fat_read_cluster(unsigned long cluster, unsigned char *buffer);
cextern dirent_t *readdir_fat(fs_node_t *node, unsigned long index);
cextern fs_node_t *finddir_fat(fs_node_t *node, const char *name);

#endif

