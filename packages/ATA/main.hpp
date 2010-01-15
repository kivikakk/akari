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

#define ATA_BUS 0x1F7
#define ATA_PRIMARY	0x1F0
#define ATA_PRIMARY_DCR	0x3F6
#define ATA_SECONDARY	0x170
#define ATA_SECONDARY_DCR	0x376

#define ATA_DATA	0x0
#define ATA_FEATURES	0x1
#define ATA_SECTOR	0x2
#define ATA_PDSA1	0x3
#define ATA_PDSA2	0x4
#define ATA_PDSA3	0x5
#define ATA_DRIVE	0x6
#define ATA_CMD		0x7

#define ATA_BSY	(1 << 7)
#define ATA_RDY (1 << 6)
#define ATA_DF	(1 << 5)
#define ATA_SRV	(1 << 4)
#define ATA_DRQ	(1 << 3)
#define ATA_ERR	(1 << 0)

#define ATA_SELECT_MASTER	0xA0
#define ATA_SELECT_SLAVE	0xB0
/* These _OP ones are used in read/write? */
#define ATA_SELECT_MASTER_OP	0xE0
#define ATA_SELECT_SLAVE_OP	0xF0

#define ATA_READ_SECTORS	0x20
#define ATA_WRITE_SECTORS	0x30
#define ATA_CACHE_FLUSH		0xE7
#define ATA_IDENTIFY		0xEC

#endif
