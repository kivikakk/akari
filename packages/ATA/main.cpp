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

#include <stdio.hpp>
#include <UserCalls.hpp>
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>
#include <debug.hpp>
#include <cpp.hpp>

#include "ATAProto.hpp"
#include "main.hpp"
#include "mindrvr.hpp"

void ata_read_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer);
void ata_write_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer);

void ata_read_sectors(u32 start, u32 number, u8 *buffer);
void ata_write_sectors(u32 start, u32 number, u8 *buffer);

extern "C" int main() {
	printf("reg_config start\n");
	int devices_found = reg_config();
	printf("reg_config complete\n");

	if (devices_found == 0) {
		printf("ATA: failed init - totally no hard drives\n");
		return 0;
	}

	printf("found %d device(s)\n", devices_found);

	// Now we need to wait and listen for commands!
	if (!registerName("system.io.ata"))
		panic("ATA: could not register system.io.ata");

	printf("[ATA] ");

	while (true) {
		struct queue_item_info info = *probeQueue();
		u8 *request = grabQueue(&info);
		shiftQueue(&info);

		if (request[0] == ATA_OP_READ) {
			ATAOpRead *op = reinterpret_cast<ATAOpRead *>(request);

			u8 *buffer = new u8[op->length];
			ata_read_data(op->sector, op->offset, op->length, buffer);
			sendQueue(info.from, info.id, buffer, op->length);
			delete [] buffer;
		} else if (request[0] == ATA_OP_WRITE) {
			ATAOpWrite *op = reinterpret_cast<ATAOpWrite *>(request);

			ata_write_data(op->sector, op->offset, op->length, op->data);
			sendQueue(info.from, info.id, reinterpret_cast<const u8 *>("\1"), 1);
		} else {
			panic("ATA: confused");
		}

		delete [] request;
	}

	panic("ATA: ran off the end of the infinite loop");
	return 1;
}

void ata_read_sectors(u32 start, u32 number, u8 *buffer) {
	while (number > 256) {
		ata_read_sectors(start, 256, buffer);
		buffer += 512 * 256;
		start += 256;
		number -= 256;
	}
	
	AkariOutB(ATA_PRIMARY + ATA_DRIVE, ATA_SELECT_MASTER_OP | ((start >> 24) & 0x0F));

	// replace with more intelligence later
	AkariInB(ATA_PRIMARY + ATA_DCR);
	AkariInB(ATA_PRIMARY + ATA_DCR);
	AkariInB(ATA_PRIMARY + ATA_DCR);
	AkariInB(ATA_PRIMARY + ATA_DCR);	
	AkariOutB(ATA_PRIMARY + ATA_FEATURES, 0x00);
	AkariOutB(ATA_PRIMARY + ATA_SECTOR, (number == 256) ? 0 : number);	// sector count
	AkariOutB(ATA_PRIMARY + ATA_PDSA1, static_cast<u8>(start & 0xFF));	// low 8 bits of LBA
	AkariOutB(ATA_PRIMARY + ATA_PDSA2, static_cast<u8>((start >> 8) & 0xFF));	// next 8
	AkariOutB(ATA_PRIMARY + ATA_PDSA3, static_cast<u8>((start >> 16) & 0xFF));	// next 8
	AkariOutB(ATA_PRIMARY + ATA_CMD, ATA_READ_SECTORS);

	u32 sectors_read = 0;
	while (sectors_read < number) {
		u8 rs = AkariInB(ATA_PRIMARY + ATA_CMD);
		while (!(!(rs & ATA_BSY) && (rs & ATA_DRQ)) && !(rs & ATA_ERR) && !(rs & ATA_DF))
			rs = AkariInB(ATA_PRIMARY + ATA_CMD);
		
		if (rs & ATA_ERR) 	panic("ATA: ATA_ERR in ata_read_sectors\n");
		else if (rs & ATA_DF) 	panic("ATA: ATA_DF in ata_read_sectors\n");

		for (u32 i = 0; i < 256; ++i) {
			u16 incoming = AkariInW(ATA_PRIMARY + ATA_DATA);
			*buffer++ = incoming & 0xff;
			*buffer++ = (incoming >> 8) & 0xff;
		}

		sectors_read++;
	}
}

void ata_read_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer)
{
	static u8 tempdata[512];

	// just in case some idiot passes in offset >= 512. why?! (that'd probably be me ..)
	u32 sector_open = sector_offset + offset / 512;
	offset %= 512;
	
	u32 sectors_read = 0;

	if (offset > 0) {
		ata_read_sectors(sector_open, 1, static_cast<u8 *>(tempdata));
		memcpy(buffer, tempdata + offset, min(static_cast<u32>(512 - offset), length));	// XXX: what was this comment about!?	/* the +1 is important, but i don't understand why */
		buffer += min(static_cast<u32>(512 - offset), length);										// XXX: and this??/* yet no +1 here */
		++sectors_read;
		length -= min(512, offset + length) - offset;
	}
	
	u32 complete_sectors = length / 512;
	if (complete_sectors > 0) {
		ata_read_sectors(sector_open + sectors_read, complete_sectors, buffer);
		sectors_read += complete_sectors;
		buffer += complete_sectors * 512;
	}
	length %= 512;

	if (length > 0) {
		ata_read_sectors(sector_open + sectors_read, 1, static_cast<u8 *>(tempdata));
		memcpy(buffer, tempdata, length);
		// buffer, sectors_read, length stale
	}
}


void ata_write_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer)
{
	static u8 tempdata[512];

	u32 sector_open, sectors_written = 0;
	u32 complete_sectors;

	sector_open = sector_offset + offset / 512;
	offset %= 512;

	if (offset > 0) {
		ata_read_sectors(sector_open, 1, static_cast<u8 *>(tempdata));
		memcpy(tempdata + offset, buffer, min(static_cast<u32>(512 - offset), length));
		ata_write_sectors(sector_open, 1, static_cast<u8 *>(tempdata));
		buffer += min(static_cast<u32>(512 - offset), length);
		++sectors_written;
		length -= min(512, offset + length) - offset;
	}

	complete_sectors = length / 512;
	if (complete_sectors > 0) {
		ata_write_sectors(sector_open + sectors_written, complete_sectors, buffer);
		sectors_written += complete_sectors;
		buffer += complete_sectors * 512;
	}
	length %= 512;

	if (length > 0) {
		ata_read_sectors(sector_open + sectors_written, 1, static_cast<u8 *>(tempdata));
		memcpy(tempdata, buffer, length);
		ata_write_sectors(sector_open + sectors_written, 1, static_cast<u8 *>(tempdata));
		/* buffer, sectors_written, length stale */
	}
}

void ata_write_sectors(u32 start, u32 number, u8 *buffer)
{
	u8 rs;
	u32 i, sectors_written = 0;

	if (number > 256)
		panic("ATA: we haven't implemented writing more than 256 sectors at once yet (> 128KiB)\n");
	else if (number == 0)
		panic("ATA: writing 0 sectors?\n");

	AkariOutB(ATA_PRIMARY + ATA_DRIVE, ATA_SELECT_MASTER_OP | ((start >> 24) & 0x0F));
	AkariInB(ATA_PRIMARY + ATA_DCR);
	AkariInB(ATA_PRIMARY + ATA_DCR);
	AkariInB(ATA_PRIMARY + ATA_DCR);
	AkariInB(ATA_PRIMARY + ATA_DCR);	/* see above */
	AkariOutB(ATA_PRIMARY + ATA_FEATURES, 0x00);
	AkariOutB(ATA_PRIMARY + ATA_SECTOR, (number == 256) ? 0 : number);
	AkariOutB(ATA_PRIMARY + ATA_PDSA1, static_cast<u8>(start & 0xFF));
	AkariOutB(ATA_PRIMARY + ATA_PDSA2, static_cast<u8>((start >> 8) & 0xFF));
	AkariOutB(ATA_PRIMARY + ATA_PDSA3, static_cast<u8>((start >> 16) & 0xFF));
	AkariOutB(ATA_PRIMARY + ATA_CMD, ATA_WRITE_SECTORS);

	while (sectors_written < number) {
		rs = AkariInB(ATA_PRIMARY + ATA_CMD);
		while (!(!(rs & ATA_BSY) && (rs & ATA_DRQ)) && !(rs & ATA_ERR) && !(rs & ATA_DF))
			rs = AkariInB(ATA_PRIMARY + ATA_CMD);
		
		if (rs & ATA_ERR) 	panic("ATA: ATA_ERR in ata_write_sectors\n");
		else if (rs & ATA_DF) 	panic("ATA: ATA_DF in ata_write_sectors\n");

		for (i = 0; i < 256; ++i) {
			AkariOutW(ATA_PRIMARY + ATA_DATA, buffer[0] | (buffer[1] << 8));
			buffer += 2;
			asm volatile("jmp .+2");
		}

		sectors_written += 1;
	}

	AkariOutB(ATA_PRIMARY + ATA_CMD, ATA_CACHE_FLUSH);
	rs = AkariInB(ATA_PRIMARY + ATA_CMD);
	while ((rs & ATA_BSY))
		rs = AkariInB(ATA_PRIMARY + ATA_CMD);
}
