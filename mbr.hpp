#ifndef __MBR_HPP__
#define __MBR_HPP__

#include <cpp.hpp>

cextern void installmbr(void);
cextern void reportmbr(void);
cextern void partition_read_data(unsigned char partition_id, unsigned long sector, unsigned short offset, unsigned long length, unsigned char *buffer);

cextern typedef struct
{
	unsigned char bootable;	/* 0x80 if bootable, 0 otherwise */
	unsigned char begin_head;
	unsigned begin_cylinderhi : 2; /* 2 high bits of start cylinder */
	unsigned begin_sector : 6;
	unsigned char begin_cylinderlo; /* low bits of start cylinder */
	unsigned char system_id;
	unsigned char end_head;
	unsigned end_cylinderhi : 2; /* 2 high bits of end cylinder */
	unsigned end_sector : 6;
	unsigned char end_cylinderlo; /* low bits of end cylinder */
	unsigned long begin_disk_sector;
	unsigned long sector_count;
} __attribute__ ((__packed__)) primary_partition_t;

cextern typedef struct
{
	char boot[446];
	primary_partition_t partitions[4];
	unsigned short signature; /* should always be 0xaa55 */
} __attribute__ ((__packed__)) master_boot_record_t;

#endif
