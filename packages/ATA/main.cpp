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

#include <UserCalls.hpp>
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>
#include <debug.hpp>

#include "ATAProto.hpp"
#include "main.hpp"

void ata_read_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer);
void ata_write_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer);

void ata_read_sectors(u32 start, u32 number, u8 *buffer);
void ata_write_sectors(u32 start, u32 number, u8 *buffer);


u32 hdd_lba28_addr;
char hdd_serial_number[21];
char hdd_firmware_revision[9];
char hdd_model_number[41];
u16 returndata[256];

bool init_drives();

extern "C" int start() {
	if (!init_drives()) {
		printf("ATA: failed init\n");
		syscall_exit();
		return 1;
	}

	// Now we need to wait and listen for commands!
	if (!syscall_registerName("system.io.ata"))
		syscall_panic("ATA: could not register system.io.ata");

	printf("[ATA] ");

	while (true) {
		struct queue_item_info info = *syscall_probeQueue();
		u8 *request = syscall_grabQueue(&info);
		syscall_shiftQueue(&info);

		if (request[0] == ATA_OP_READ) {
			ATAOpRead *op = reinterpret_cast<ATAOpRead *>(request);

			u8 *buffer = new u8[op->length];
			ata_read_data(op->sector, op->offset, op->length, buffer);
			syscall_sendQueue(info.from, info.id, buffer, op->length);
			delete [] buffer;
		} else if (request[0] == ATA_OP_WRITE) {
			ATAOpWrite *op = reinterpret_cast<ATAOpWrite *>(request);

			ata_write_data(op->sector, op->offset, op->length, op->data);
			syscall_sendQueue(info.from, info.id, reinterpret_cast<const u8 *>("\1"), 1);
		} else {
			syscall_panic("ATA: confused");
		}

		delete [] request;
	}

	syscall_panic("ATA: ran off the end of the infinite loop");
	return 1;
}

bool init_drives() {
	u8 rs = AkariInB(ATA_BUS);
	if (rs == 0xff) {
		printf("ATA: floating bus! No hard drives?\n");
		return false;
	}

	AkariOutB(ATA_PRIMARY + ATA_DRIVE, ATA_SELECT_MASTER);
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);
	AkariOutB(ATA_PRIMARY + ATA_CMD, ATA_IDENTIFY);

	rs = AkariInB(ATA_PRIMARY + ATA_CMD);
	if (rs == 0) {
		printf("ATA: no hard drives.\n");
		return false;
	}

	while (rs & ATA_BSY)
		rs = AkariInB(ATA_PRIMARY + ATA_CMD);
	
	if (rs & ATA_ERR)
		syscall_panic("ATA: error on IDENTIFY.\n");
	else if (!(rs & ATA_DRQ))
		syscall_panic("ATA: no error on IDENTIFY, but no DRQ?\n");

	for (u32 i = 0; i < 256; ++i)
		returndata[i] = AkariInW(ATA_PRIMARY + ATA_DATA);
	
	for (u32 i = 10; i < 20; ++i) {
		hdd_serial_number[(i - 10) * 2] = (returndata[i] >> 8) & 0xff;
		hdd_serial_number[(i - 10) * 2 + 1] = returndata[i] & 0xff;
	}
	hdd_serial_number[20] = 0;

	for (u32 i = 23; i < 27; ++i) {
		hdd_firmware_revision[(i - 23) * 2] = (returndata[i] >> 8) & 0xff;
		hdd_firmware_revision[(i - 23) * 2 + 1] = returndata[i] & 0xff;
	}
	hdd_firmware_revision[8] = 0;

	for (u32 i = 27; i < 47; ++i) { 
		hdd_model_number[(i - 27) * 2] = (returndata[i] >> 8) & 0xff;
		hdd_model_number[(i - 27) * 2 + 1] = returndata[i] & 0xff;
	}
	hdd_model_number[40] = 0;

	if (!(returndata[60] || returndata[61]))
		syscall_panic("ATA: hard drive does not support LBA28?");

	hdd_lba28_addr = returndata[60] | (returndata[61] << 16);

	/* LBA48
	printf("\nSupports LBA48 mode: ");
	printf((returndata[83] & (1 << 10)) ? "yes" : "no");
	printf("\nAddressable LBA48 sectors: ");
	li = returndata[100] | (returndata[101] << 16) | ((u32 long)returndata[102] << 32) | ((u32 long)returndata[103] << 48);
	puthexlong(li); printf(" ("); putdec(li * 512 / 1024); printf(" KiB)");
	 */

	return true;
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
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);	
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
		
		if (rs & ATA_ERR) 	syscall_panic("ATA: ATA_ERR in ata_read_sectors\n");
		else if (rs & ATA_DF) 	syscall_panic("ATA: ATA_DF in ata_read_sectors\n");

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
		syscall_memcpy(buffer, tempdata + offset, min(static_cast<u32>(512 - offset), length));	// XXX: what was this comment about!?	/* the +1 is important, but i don't understand why */
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
		syscall_memcpy(buffer, tempdata, length);
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
		syscall_memcpy(tempdata + offset, buffer, min(static_cast<u32>(512 - offset), length));
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
		syscall_memcpy(tempdata, buffer, length);
		ata_write_sectors(sector_open + sectors_written, 1, static_cast<u8 *>(tempdata));
		/* buffer, sectors_written, length stale */
	}
}

void ata_write_sectors(u32 start, u32 number, u8 *buffer)
{
	u8 rs;
	u32 i, sectors_written = 0;

	if (number > 256)
		syscall_panic("ATA: we haven't implemented writing more than 256 sectors at once yet (> 128KiB)\n");
	else if (number == 0)
		syscall_panic("ATA: writing 0 sectors?\n");

	AkariOutB(ATA_PRIMARY + ATA_DRIVE, ATA_SELECT_MASTER_OP | ((start >> 24) & 0x0F));
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);	/* see above */
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
		
		if (rs & ATA_ERR) 	syscall_panic("ATA: ATA_ERR in ata_write_sectors\n");
		else if (rs & ATA_DF) 	syscall_panic("ATA: ATA_DF in ata_write_sectors\n");

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
