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

inline u32 min(u32 a, u32 b) {
	if (a > b) return b;
	return a;
}

void ata_read_sectors(u32 start, u32 number, u8 *buffer);
void ata_write_sectors(u32 start, u32 number, u8 *buffer);

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

void ata_read_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer);
void ata_write_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer);

void ATAProcess() {
	u32 hdd_lba28_addr;
	char hdd_serial_number[21];
	char hdd_firmware_revision[9];
	char hdd_model_number[41];

	/* LBA48: u32 long li; */
	u16 returndata[256];

	u8 rs = AkariInB(ATA_BUS);
	if (rs == 0xff) {
		syscall_puts("Floating bus! No hard drives?\n");
		return;
	}

	AkariOutB(ATA_PRIMARY + ATA_DRIVE, ATA_SELECT_MASTER);
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);
	AkariOutB(ATA_PRIMARY + ATA_CMD, ATA_IDENTIFY);

	rs = AkariInB(ATA_PRIMARY + ATA_CMD);
	if (rs == 0) {
		syscall_puts("No hard drives.\n");
		return;
	}

	while (rs & ATA_BSY)
		rs = AkariInB(ATA_PRIMARY + ATA_CMD);
	
	if (rs & ATA_ERR)
		syscall_panic("ATA error on IDENTIFY.\n");
	else if (!(rs & ATA_DRQ))
		syscall_panic("No error on ATA IDENTIFY, but no DRQ?\n");

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
		syscall_panic("Hard drive does not support LBA28?");

	hdd_lba28_addr = returndata[60] | (returndata[61] << 16);

	/* LBA48
	syscall_puts("\nSupports LBA48 mode: ");
	syscall_puts((returndata[83] & (1 << 10)) ? "yes" : "no");
	syscall_puts("\nAddressable LBA48 sectors: ");
	li = returndata[100] | (returndata[101] << 16) | ((u32 long)returndata[102] << 32) | ((u32 long)returndata[103] << 48);
	puthexlong(li); syscall_puts(" ("); putdec(li * 512 / 1024); syscall_puts(" KiB)");
	 */

	// Now we need to wait and listen for commands!
	if (!syscall_registerName("system.io.ata"))
		syscall_panic("could not register system.io.ata");

	syscall_puts("ATA driver entering loop\n");
#define ATA_BUFFER 10240
	char buffer[ATA_BUFFER];
	while (true) {
		struct queue_item_info *info = syscall_probeQueue();
		u32 len = info->data_len;
		if (len > ATA_BUFFER) syscall_panic("ATA buffer overflow");
		syscall_readQueue(buffer, 0, len);

		if (buffer[0] == 0) {
			// Read

			u32 sector_offset = buffer[1] << 24 | buffer[2] << 16 | buffer[3] << 8 | buffer[4];
			u16 offset = buffer[5] << 8 | buffer[6];
			u32 length = buffer[7] << 24 | buffer[8] << 16 | buffer[9] << 8 | buffer[10];

			if (length > ATA_BUFFER) syscall_panic("Static buffer in ATA too small for requested amount");

			ata_read_data(sector_offset, offset, length, reinterpret_cast<u8 *>(buffer));

			syscall_sendQueue(info->from, info->id, buffer, length);

		} else if (buffer[0] == 1) {
			// Write

			u32 sector_offset = buffer[1] << 24 | buffer[2] << 16 | buffer[3] << 8 | buffer[4];
			u16 offset = buffer[5] << 8 | buffer[6];
			u32 length = buffer[7] << 24 | buffer[8] << 16 | buffer[9] << 8 | buffer[10];

			if (len - 11 > length) syscall_panic("ATA driver got more data in msg than msg asked to write");
			if (len - 11 < length) syscall_panic("ATA driver got LESS data in msg than msg asked to write!");

			ata_write_data(sector_offset, offset, length, reinterpret_cast<u8 *>(buffer + 11));

			syscall_sendQueue(info->from, info->id, "\1", 1);
		} else {
			syscall_panic("ATA driver confused");
		}

		syscall_shiftQueue();
	}
}

void ata_read_data(u32 sector_offset, u16 offset, u32 length, u8 *buffer)
{
	static u8 tempdata[512];

	/* Don't forget that uninitialised values have random values in C. */
	u32 sector_open, sectors_read = 0;
	u32 complete_sectors;

	/* just in case some idiot passes in offset >= 512. why?! */
	sector_open = sector_offset + offset / 512;
	offset %= 512;
	
	if (offset > 0) {
		ata_read_sectors(sector_open, 1, static_cast<u8 *>(tempdata));
		syscall_memcpy(buffer, tempdata + offset, min(static_cast<u32>(512 - offset), length));	// XXX: what was this comment about!?	/* the +1 is important, but i don't understand why */
		buffer += min(static_cast<u32>(512 - offset), length);										// XXX: and this??/* yet no +1 here */
		++sectors_read;
		length -= min(512, offset + length) - offset;
	}
	
	complete_sectors = length / 512;
	if (complete_sectors > 0) {
		ata_read_sectors(sector_open + sectors_read, complete_sectors, buffer);
		sectors_read += complete_sectors;
		buffer += complete_sectors * 512;
	}
	length %= 512;

	if (length > 0) {
		ata_read_sectors(sector_open + sectors_read, 1, static_cast<u8 *>(tempdata));
		syscall_memcpy(buffer, tempdata, length);
		/* buffer, sectors_read, length stale */
	}
}

void ata_read_sectors(u32 start, u32 number, u8 *buffer)
{
	u8 rs;
	u16 returndata;
	u32 i, sectors_read = 0;

	while (number > 256) {
		ata_read_sectors(start, 256, buffer);
		buffer += 512 * 256;
		start += 256;
		number -= 256;
	}
	
	if (number == 0)
		syscall_panic("Reading 0 sectors?\n");

	AkariOutB(ATA_PRIMARY + ATA_DRIVE, ATA_SELECT_MASTER_OP | ((start >> 24) & 0x0F));
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);
	AkariInB(ATA_PRIMARY_DCR);	/* replace with more intelligence later */
	AkariOutB(ATA_PRIMARY + ATA_FEATURES, 0x00);
	AkariOutB(ATA_PRIMARY + ATA_SECTOR, (number == 256) ? 0 : number);	/* sector count */
	AkariOutB(ATA_PRIMARY + ATA_PDSA1, static_cast<u8>(start & 0xFF));	/* low 8 bits of LBA */
	AkariOutB(ATA_PRIMARY + ATA_PDSA2, static_cast<u8>((start >> 8) & 0xFF));	/* next 8 */
	AkariOutB(ATA_PRIMARY + ATA_PDSA3, static_cast<u8>((start >> 16) & 0xFF));	/* next 8 */
	AkariOutB(ATA_PRIMARY + ATA_CMD, ATA_READ_SECTORS);

	while (sectors_read < number) {
		rs = AkariInB(ATA_PRIMARY + ATA_CMD);
		while (!(!(rs & ATA_BSY) && (rs & ATA_DRQ)) && !(rs & ATA_ERR) && !(rs & ATA_DF))
			rs = AkariInB(ATA_PRIMARY + ATA_CMD);
		
		if (rs & ATA_ERR) 	syscall_panic("ATA_ERR in ata_read_sectors\n");
		else if (rs & ATA_DF) 	syscall_panic("ATA_DF in ata_read_sectors\n");

		for (i = 0; i < 256; ++i) {
			returndata = AkariInW(ATA_PRIMARY + ATA_DATA);
			*buffer++ = returndata & 0xff;
			*buffer++ = (returndata >> 8) & 0xff;
		}

		sectors_read++;
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
		syscall_panic("We haven't implemented writing more than 256 sectors at once yet (> 128KiB)\n");
	else if (number == 0)
		syscall_panic("Writing 0 sectors?\n");

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
		
		if (rs & ATA_ERR) 	syscall_panic("ATA_ERR in ata_write_sectors\n");
		else if (rs & ATA_DF) 	syscall_panic("ATA_DF in ata_write_sectors\n");

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

