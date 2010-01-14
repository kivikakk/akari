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

#ifndef MAIN_HPP
#define MAIN_HPP

#define MBR_MAX_WILL_ALLOC 0x80000

typedef struct
{
	u8 bootable;	/* 0x80 if bootable, 0 otherwise */
	u8 begin_head;
	unsigned begin_cylinderhi : 2; /* 2 high bits of start cylinder */
	unsigned begin_sector : 6;
	u8 begin_cylinderlo; /* low bits of start cylinder */
	u8 system_id;
	u8 end_head;
	unsigned end_cylinderhi : 2; /* 2 high bits of end cylinder */
	unsigned end_sector : 6;
	u8 end_cylinderlo; /* low bits of end cylinder */
	u32 begin_disk_sector;
	u32 sector_count;
} __attribute__ ((__packed__)) primary_partition_t;

typedef struct
{
	u8 boot[446];
	primary_partition_t partitions[4];
	u16 signature; /* should always be 0xaa55 */
} __attribute__ ((__packed__)) master_boot_record_t;

#endif
