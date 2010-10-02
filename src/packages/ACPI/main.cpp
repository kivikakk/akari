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

#include "main.hpp"

bool acpiInit();

extern "C" int main() {
	printf("ACPI: starting ... ");

	acpiInit();

	printf("done!\n");

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

			// send a confirmation that it's done.
			sendQueue(info.from, info.id, 0, 0);
		} else {
			panic("ATA: confused");
		}

		delete [] request;
	}
}

bool acpiInit() {
	u32 *ptr = acpiGetRSDPtr();

	if (ptr != 0 && acpiCheckHeader(ptr, "RSDT") == 0) {
		// we have ACPI.
		int entries = *(ptr + 1);
		entries = (entries - 36) / 4;
		ptr += 36 / 4;

		while (0 < entries--) {
			if (acpiCheckHeader((u32 *)*ptr, "FACP") == 0) {
				entries = -2;
				struct FACP *facp = reinterpret_cast<FACP *>(*ptr);
				if (acpiCheckHeader((u32 *)facp->DSDT, "DSDT") == 0) {
					// Find the \_S5 package in the DSDT
					u8 *S5addr = (u8 *)facp->DSDT + 36;
					int dsdtLength = *(facp->DSDT + 1) - 36;
					while (0 < dsdtLength--) {
						if (memcmp(S5addr, "_S5_", 4) == 0)
							break;
						S5addr++;
					}

					if (dsdtLength > 0) {
						if ((*(S5addr - 1) == 0x08 || (*(S5addr - 2) == 0x08 && *(S5addr - 1) == '\\')) && *(S5addr + 4) == 0x12) {
							S5addr += 5;
							S5addr += ((*S5addr & 0xC0) >> 6) + 2;

							if (*S5addr == 0x0A)
								S5addr++;
							SLP_TYPb = *(S5addr) << 10;

							SMI_CMD = facp->SMI_CMD;

							ACPI_ENABLE = facp->ACPI_ENABLE;
							ACPI_DISABLE = facp->ACPI_DISABLE;

							PM1a_CNT = facp->PM1a_CNT_BLK;
							PM1b_CNT = facp->PM1b_CNT_BLK;

							PM1_CNT_LEN = facp->PM1_CNT_LEN;

							SLP_EN = 1 << 13;
							SCI_EN = 1;

							return true;
						} else {
							printf("\\_S5 parse error.\n");
						}
					} else {
						printf("\\_S5 not present.\n");
					}
				} else {
					printf("DSDT invalid.\n");
				}
			}
			++ptr;
		}

		printf("no valid FACP present.\n");
	} else {
		printf("no ACPI.\n");
	}

	return false;
}
