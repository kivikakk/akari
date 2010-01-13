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
