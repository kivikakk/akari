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

#include <mbr.hpp>
#include <ata.hpp>
#include <panic.hpp>
#include <screen.hpp>

static master_boot_record_t hdd_mbr;

void installmbr(void)
{
	ata_read_data(0, 0, 512, (unsigned char *)&hdd_mbr);

	if (hdd_mbr.signature != 0xaa55)
		panic("Invalid MBR!\n");
}

void reportmbr(void)
{
	int p;

	puts("MBR partitions:\n");
	puts("start    length   type\n");
	puts("-----    ------   ----\n");
	for (p = 0; p < 4; p++)
	{
		if (hdd_mbr.partitions[p].system_id == 0)
			continue;
		puthexlong(hdd_mbr.partitions[p].begin_disk_sector);
		puts(" ");
		puthexlong(hdd_mbr.partitions[p].sector_count);
		puts(" ");
		puthexchar(hdd_mbr.partitions[p].system_id);
		puts("\n");
	}
}

void partition_read_data(unsigned char partition_id, unsigned long sector, unsigned short offset, unsigned long length, unsigned char *buffer)
{
	return ata_read_data(hdd_mbr.partitions[partition_id].begin_disk_sector + sector, offset, length, buffer);
}
