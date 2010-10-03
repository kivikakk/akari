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
#include <string>
#include <time.hpp>

#include "main.hpp"
#include "proto.hpp"

u32 *SMI_CMD;
u8 ACPI_ENABLE, ACPI_DISABLE;
u32 *PM1a_CNT, *PM1b_CNT;
u16 SLP_TYPa, SLP_TYPb, SLP_EN, SCI_EN;
u8 PM1_CNT_LEN;

extern "C" int main() {
	acpiInit();

	while (true) {
		struct queue_item_info info = *probeQueue();
		u8 *request = grabQueue(&info);
		shiftQueue(&info);

		if (request[0] == ACPI_OP_SHUTDOWN) {
			acpiPowerOff();
		} else if (request[0] == ACPI_OP_REBOOT) {
			acpiReboot();
		} else {
			panic("ACPI: confused");
		}

		delete [] request;
	}
}

bool acpiInit() {
	u32 *ptr = acpiGetRSDPtr();
	pid_t self = processId();
	mapPhysicalMem(self, (u32)ptr & 0xFFFFF000, ((u32)ptr & 0xFFFFF000) + 0x2000, (u32)ptr & 0xFFFFF000, true);

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

							grantIOPriv(self, (u16)(u32)SMI_CMD);
							grantIOPriv(self, (u16)(u32)PM1a_CNT);
							grantIOPriv(self, (u16)(u32)PM1a_CNT + 1);
							grantIOPriv(self, (u16)(u32)PM1b_CNT);
							grantIOPriv(self, (u16)(u32)PM1b_CNT + 1);

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

int acpiCheckHeader(u32 *ptr, const char *sig) {
	if (memcmp(ptr, sig, 4) == 0) {
		u8 *checkPtr = reinterpret_cast<u8 *>(ptr);
		int len = *(ptr + 1);
		u8 check = 0;
		while (0 < len--) {
			check += *checkPtr;
			++checkPtr;
		}
		if (check == 0)
			return 0;
	}
	return -1;
}

u32 *acpiGetRSDPtr() {
	u32 *addr, *rsdp;

	for (addr = reinterpret_cast<u32 *>(0x000E0000); addr < reinterpret_cast<u32 *>(0x00100000); addr += 0x10 / sizeof(addr)) {
		rsdp = acpiCheckRSDPtr(addr);
		if (rsdp != 0)
			return rsdp;
	}

	printf("ACPI: trying to fetch from edba\n");
	int ebda = *((u16 *)0x40E);
	ebda = ebda * 0x10 & 0x000FFFFF;

	for (addr = reinterpret_cast<u32 *>(ebda); addr < reinterpret_cast<u32 *>(ebda + 1024); addr += 0x10 / sizeof(addr)) {
		rsdp = acpiCheckRSDPtr(addr);
		if (rsdp != 0)
			return rsdp;
	}

	return 0;
}

u32 *acpiCheckRSDPtr(u32 *ptr) {
	const char *sig = "RSD PTR ";
	struct RSDPtr *rsdp = reinterpret_cast<struct RSDPtr *>(ptr);
	u8 *bptr;
	u8 check = 0;
	int i;

	if (memcmp(sig, rsdp, 8) == 0) {
		bptr = (u8 *)ptr;
		for (i = 0; i < static_cast<int>(sizeof(struct RSDPtr)); ++i) {
			check += *bptr;
			++bptr;
		}

		if (check == 0) {
			return (u32 *)rsdp->RsdtAddress;
		}
	}

	return 0;
}

bool acpiEnable() {
	if ((AkariInW((u32)PM1a_CNT) & SCI_EN) == 0) {
		if (SMI_CMD != 0 && ACPI_ENABLE != 0) {
			AkariOutB((u32)SMI_CMD, ACPI_ENABLE);

			int i;
			for (i = 0; i < 300; ++i) {
				if ((AkariInW((u32)PM1a_CNT) & SCI_EN) == 1)
					break;
				usleep(100000);
			}
			if (PM1b_CNT != 0) {
				for (; i < 300; ++i) {
					if ((AkariInW((u32)PM1b_CNT) & SCI_EN) == 1)
						break;
					usleep(100000);
				}
			}
			if (i < 300) {
				printf("enabled ACPI\n");
				return true;
			} else {
				printf("couldn't enable ACPI\n");
				return false;
			}
		} else {
			printf("no known way to enable ACPI\n");
			return false;
		}
	} else {
		printf("ACPI already enabled\n");
		return true;
	}
}

void acpiPowerOff() {
	// SCI_EN set to 1 if ACPI shutdown is possible
	if (SCI_EN == 0)
		return;

	acpiEnable();

	AkariOutW((u32)PM1a_CNT, SLP_TYPa | SLP_EN);
	if (PM1b_CNT != 0)
		AkariOutW((u32)PM1b_CNT, SLP_TYPb | SLP_EN);
	
	printf("ACPI poweroff failed.\n");
}

void acpiReboot() {
	// NOTE: we don't even use ACPI for this!
	sysreboot();
}
