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

#include "PCIProto.hpp"
#include "main.hpp"

bool init();
u16 check_vendor(u16 bus, u16 slot, u16 fn);

extern "C" int main() {
	if (!init()) {
		printf("PCI: failed init\n");
		return 1;
	}

	// Now we need to wait and listen for commands!
	if (!registerName("system.bus.pci"))
		panic("PCI: could not register system.bus.cpi");

	printf("[PCI]\n");

	for (u32 bus = 0; bus < 256; ++bus) {
		for (u32 device = 0; device < 32; ++device) {
			for (u32 fn = 0; fn < 8; ++fn) {
				u16 cv = check_vendor(bus, device, fn);

				if (fn == 0 && cv == 0xFFFF) {
					break;
				}
			}
		}
	}

	printf("enumeration complete\n");

	while (true) {
		struct queue_item_info info = *probeQueue();
		u8 *request = grabQueue(&info);
		shiftQueue(&info);

		/*
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

			u8 *buffer = new u8[op->length];
			partition_read_data(op->partition_id, op->sector, op->offset, op->length, buffer);
			sendQueue(info.from, info.id, buffer, op->length);
			delete [] buffer;
		} else {
			panic("ATA: confused");
		}
		*/

		delete [] request;
	}

	panic("PCI: ran off the end of the infinite loop");
	return 1;
}

u16 read_config_word(u16 bus, u16 slot, u16 fn, u16 offset) {
	pci_config_cycle pcc = { 0, offset & 0xFC, fn, slot, bus, 0, 1 };
	AkariOutL(CONFIG_ADDRESS, *(reinterpret_cast<u32 *>(&pcc)));
	return ((AkariInL(CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
}

u16 check_vendor(u16 bus, u16 slot, u16 fn) {
	u16 vendor = read_config_word(bus, slot, fn, 0);
	u16 device;

	if (vendor != 0xFFFF) {
		device = read_config_word(bus, slot, fn, 2);
		printf("%x/%x/%x: vendor %x, device %x\n", bus, slot, fn, vendor, device);
	}

	return vendor;
}

bool init() {
	return true;
}

