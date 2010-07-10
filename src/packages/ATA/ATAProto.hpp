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

#ifndef ATA_PROTO_HPP
#define ATA_PROTO_HPP

#define ATA_OP_READ 		0x1
#define ATA_OP_WRITE		0x2
#define ATA_OP_MBR_READ		0x3
#define ATA_OP_SET_BAR4		0x4

typedef struct {
	u8 cmd;
	u32 sector;
	u16 offset;
	u32 length;
} ATAOpRead;

typedef struct {
	u8 cmd;
	u32 sector;
	u16 offset;
	u32 length;
	u8 data[];
} ATAOpWrite;

typedef struct {
	u8 cmd;
	u8 partition_id;
	u32 sector;
	u16 offset;
	u32 length;
} ATAOpMBRRead;

typedef struct {
	u8 cmd;
	u32 bar4;
} ATAOpSetBAR4;

#endif
