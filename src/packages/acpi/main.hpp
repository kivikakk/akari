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

#include <arch.hpp>

struct RSDPtr {
	u8 Signature[8];
	u8 CheckSum;
	u8 OemID[6];
	u8 Revision;
	u32 *RsdtAddress;
};

struct FACP {
	u8 Signature[4];
	u32 Length;
	u8 unneeded1[40 - 8];
	u32 *DSDT;
	u8 unneeded2[48 - 44];
	u32 *SMI_CMD;
	u8 ACPI_ENABLE;
	u8 ACPI_DISABLE;
	u8 unneeded3[64 - 54];
	u32 *PM1a_CNT_BLK;
	u32 *PM1b_CNT_BLK;
	u8 unneeded4[89 - 72];
	u8 PM1_CNT_LEN;
};

bool acpiInit();
int acpiCheckHeader(u32 *ptr, const char *sig);
u32 *acpiGetRSDPtr();
u32 *acpiCheckRSDPtr(u32 *ptr);
bool acpiEnable();
void acpiPowerOff();
void acpiReboot();

#endif
