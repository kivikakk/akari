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

void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, u8 *buffer);

void ata_read_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer);
void ata_write_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer);

void ata_read_sectors(u32 start, u32 number, u8 *buffer);
void ata_write_sectors(u32 start, u32 number, u8 *buffer);

master_boot_record_t hdd_mbr;

bool dma_enabled = false;

extern "C" int main() {
	int devices_found = reg_config();

	if (devices_found == 0) {
		printf("ATA: failed init - totally no hard drives\n");
		return 0;
	}

	ata_read_data(0, 0, 512, reinterpret_cast<u8 *>(&hdd_mbr));
	if (hdd_mbr.signature != 0xAA55) panic("MBR: invalid MBR!\n");

	// Now we need to wait and listen for commands!
	if (!registerName("system.io.ata"))
		panic("ATA: could not register system.io.ata");

	printf("[ATA]\n");

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
		} else if (request[0] == ATA_OP_MBR_READ) {
			ATAOpMBRRead *op = reinterpret_cast<ATAOpMBRRead *>(request);

			// u8 *buffx = new u8[1024];
			// partition_read_data(0, 0, 0, 16, buffx);
			// printf("BUFFX read 1024: %s/%x %x %x %x %x %x %x %x ...\n", buffx, buffx[0], buffx[1], buffx[2], buffx[3], buffx[4], buffx[5], buffx[6], buffx[7]);
			// delete [] buffx;

			u8 *buffer = new u8[op->length];
			partition_read_data(op->partition_id, op->sector, op->offset, op->length, buffer);
			sendQueue(info.from, info.id, buffer, op->length);
			delete [] buffer;
		} else if (request[0] == ATA_OP_SET_BAR4) {
			ATAOpSetBAR4 *op = reinterpret_cast<ATAOpSetBAR4 *>(request);

			pio_bmide_base_addr = op->bar4;
			dma_pci_config();
			dma_enabled = true;

			printf("ATA: bar4 set to %x\n", op->bar4);

			// u8 *buffer = new u8[1024];
			static u8 buffer[1024];
			partition_read_data(0, 0, 0, 511, buffer);

			printf("read 1024: %x %s/%x %x %x %x %x %x %x %x ...\n", buffer, buffer, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);

			// delete [] buffer;
		} else {
			panic("ATA: confused");
		}

		delete [] request;
	}

	panic("ATA: ran off the end of the infinite loop");
	return 1;
}

void partition_read_data(u8 partition_id, u32 sector, u16 offset, u32 length, u8 *buffer) {
	u32 new_sector = hdd_mbr.partitions[partition_id].begin_disk_sector + sector;
	ata_read_data(new_sector, offset, length, buffer);
}

void ata_read_sectors(u32 start, u32 number, u8 *buffer) {
	/* I'm not sure if the code below will do what's anticipated of it.
	 * sc will be clipped to 8 bits. Could mean nasty stuff, seeing as
	 * we're banking on the default "0 means 256", but you know, 3 means 3.
	 *
	 * e.g. If we transfer 259 sectors, (u8)259==(u8)3, so a call like:
	 * reg_pio_data_in_lba28(0, CMD_READ_SECTORS, 0, 259, start, buf, 259, 0)
	 * .. will eventually set sc to 3 in the command.
	 * This next bit here might buffer some of the impact by limiting our
	 * calls to mindrvr to doing 256 sectors at a time.
	 */

	// Definitive answer:
	//   I've concluded that, seeing as sc is passed straight through to
	// the sector count port, we need to manage its value ourself.
	//   number > 256 in the reg_pio_data_in_lba28 call means sc>256, but
	// it gets truncated to a u8 and thus havoc ensures trying to read in
	// 1025 sectors when each read call gives you 1 and not the 256 you
	// were hoping for, save the last time.
	//   mindrvr doesn't mitigate this situation for us, since the actual
	// reg_pio_data_in_lba28 call can be used with multiCnt>1 and a cmd
	// like CMD_READ_MULTIPLE - i.e. the caller is expected to know ATA.
	// (otherwise we wouldn't have to pass in a cmd!)

	// printf("ata_read_sectors; initial call num %d buffer %x\n", number, buffer);
	while (number > 256) {
		ata_read_sectors(start, 256, buffer);
		buffer += 512 * 256;
		start += 256;
		number -= 256;
	}
	// printf("ata_read_sectors; settled on call num %d buffer %x\n", number, buffer);


	if (!dma_enabled) {
	// printf("ata_read_sectors(start: %d, number: %d, buffer: 0x%x)\n", start, number, buffer);
		reg_pio_data_in_lba28(0, CMD_READ_SECTORS, 0, number, start, buffer, number, 0);
	} else {
		phptr phys;
		u8 *linear = reinterpret_cast<u8 *>(mallocap(number * 512, &phys));

		u32 dma_result = dma_pci_lba28(0, CMD_READ_DMA, 0, number, start, phys, number);

		printf(">>> (ii) dma pci lba28: num %d, start %d, buf %x, result: %d\n", number, start, phys, dma_result);
		printf(">>> (iii) linear is %x, starts %s/%x %x %x %x\n", linear, linear, *linear, linear[1], linear[2], linear[3]);

		printf("bytesXfer: %d, ec: \n", reg_cmd_info.totalBytesXfer, reg_cmd_info.ec);
		memcpy(buffer, linear, number * 512);
	}
}

void ata_read_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer) {
	static u8 tempdata[512];

	// just in case some idiot passes in offset >= 512. why?! (that'd probably be me ..)
	u32 sector_open = sector_offset + offset / 512;
	offset %= 512;
	
	u32 sectors_read = 0;

	if (offset > 0) {
		// Since the start of our read isn't on a sector boundary, we need to grab
		// that whole sector, then just copy starting from the relevant position to
		// our buffer.
		ata_read_sectors(sector_open, 1, static_cast<u8 *>(tempdata));
		memcpy(buffer, tempdata + offset, min(static_cast<u32>(512 - offset), length));
		buffer += min(static_cast<u32>(512 - offset), length);
		++sectors_read;
		length -= min(512, offset + length) - offset;
	}
	
	u32 complete_sectors = length / 512;
	if (complete_sectors > 0) {
		// We have some number of complete sectors which we can read out wholesale.
		ata_read_sectors(sector_open + sectors_read, complete_sectors, buffer);

		sectors_read += complete_sectors;
		buffer += complete_sectors * 512;
	}
	length %= 512;

	if (length > 0) {
		// There's some data left over which don't make up a whole sector. Read the
		// last sector in, then copy over the appropriate portion to our buffer.
		// We don't update our counters (as we do in the start-case) as we don't
		// need them again.
		ata_read_sectors(sector_open + sectors_read, 1, static_cast<u8 *>(tempdata));
		memcpy(buffer, tempdata, length);
		// buffer, sectors_read, length stale
	}
}


void ata_write_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer) {
	static u8 tempdata[512];

	u32 sector_open, sectors_written = 0;
	u32 complete_sectors;

	sector_open = sector_offset + offset / 512;
	offset %= 512;

	if (offset > 0) {
		// Since we're not starting our data write at a sector-aligned place,
		// we have to read in the sector, then copy our change over the portion
		// of it that's different, then write out that whole sector.

		ata_read_sectors(sector_open, 1, static_cast<u8 *>(tempdata));
		memcpy(tempdata + offset, buffer, min(static_cast<u32>(512 - offset), length));
		ata_write_sectors(sector_open, 1, static_cast<u8 *>(tempdata));
		buffer += min(static_cast<u32>(512 - offset), length);
		++sectors_written;
		length -= min(512, offset + length) - offset;
	}

	complete_sectors = length / 512;
	if (complete_sectors > 0) {
		// We have complete sectors which can be written out without any
		// funny business.
		ata_write_sectors(sector_open + sectors_written, complete_sectors, buffer);
		sectors_written += complete_sectors;
		buffer += complete_sectors * 512;
	}
	length %= 512;

	if (length > 0) {
		// Grab the last sector, put our modifications on top, rewrite (as above).
		// We don't update our counters like we do last time, since we don't need
		// them again.
		ata_read_sectors(sector_open + sectors_written, 1, static_cast<u8 *>(tempdata));
		memcpy(tempdata, buffer, length);
		ata_write_sectors(sector_open + sectors_written, 1, static_cast<u8 *>(tempdata));
		// buffer, sectors_written, length stale
	}
}

void ata_write_sectors(u32 start, u32 number, u8 *buffer) {
	// See ata_read_sectors for a big rant on mindrvr usage.
	
	while (number > 256) {
		ata_write_sectors(start, 256, buffer);
		buffer += 512 * 256;
		start += 256;
		number -= 256;
	}

	reg_pio_data_out_lba28(0, CMD_WRITE_SECTORS, 0, number, start, buffer, number, 0);
}
