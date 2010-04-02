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
#include <fs.hpp>
#include <proc.hpp>
#include <list>
#include <map>
#include <algorithm>

#include "PCIProto.hpp"
#include "main.hpp"

#define VENDOR_DEVICE(vendor, device) (((u32)((u16)(vendor)) << 16) | (u16)(device))

typedef u32 device_t;
typedef std::list<device_t> device_list_t;
typedef std::map<pid_t, device_list_t> auth_map;
static auth_map auths;

static bool init();
static u16 check_vendor(u16 bus, u8 slot, u8 fn);
static void check_device(u16 bus, u8 slot, u8 fn, u16 vendor);
static u16 read_config_word(u16 bus, u8 slot, u8 fn, u16 offset);
static u32 read_config_long(u16 bus, u8 slot, u8 fn, u16 offset);
static void add_auth(pid_t pid, u16 vendor, u16 device);
static bool is_authed(pid_t pid, u16 vendor, u16 device);

extern "C" int main() {
	if (!init()) {
		printf("PCI: failed init\n");
		return 1;
	}

	// Now we need to wait and listen for commands!
	if (!registerName("system.bus.pci"))
		panic("PCI: could not register system.bus.pci");

	printf("[PCI]\n");

	while (true) {
		struct queue_item_info info = *probeQueue();
		u8 *request = grabQueue(&info);
		shiftQueue(&info);

		if (request[0] == PCI_OP_DEVICE_CONFIG) {
			PCIOpDeviceConfig *op = reinterpret_cast<PCIOpDeviceConfig *>(request);

			if (!is_authed(info.from, op->vendor, op->device)) {
				printf("PCI: unauth'd request?\n");
				sendQueue(info.from, info.id, 0, 0);
			} else {
				pci_device_regular pciinfo;
				for (u32 i = 0; i < sizeof(pciinfo); i += 4) {
					*(reinterpret_cast<u32 *>(&pciinfo) + (i / 4)) = read_config_long(op->bus, op->slot, op->fn, i);
				}

				sendQueue(info.from, info.id, reinterpret_cast<u8 *>(&pciinfo), sizeof(pciinfo));
			}
		} else {
			panic("PCI: confused");
		}

		delete [] request;
	}

	panic("PCI: ran off the end of the infinite loop");
	return 1;
}

bool init() {
	for (u16 bus = 0; bus < 256; ++bus) {
		for (u8 slot = 0; slot < 32; ++slot) {
			for (u8 fn = 0; fn < 8; ++fn) {
				u16 vendor = check_vendor(bus, slot, fn);

				if (fn == 0 && vendor == 0xFFFF) {
					break;
				} else if (vendor != 0xFFFF) {
					check_device(bus, slot, fn, vendor);
				}
			}
		}
	}

	return true;
}

u16 check_vendor(u16 bus, u8 slot, u8 fn) {
	return read_config_word(bus, slot, fn, 0);
}

void check_device(u16 bus, u8 slot, u8 fn, u16 vendor) {
	u16 device = read_config_word(bus, slot, fn, 2);
	printf("%x/%x/%x: vendor %x, device %x\n", bus, slot, fn, vendor, device);

	char *filename = rasprintf("/%4x%4x", vendor, device);

	if (fexists(filename)) {
		// pci_device_regular pciinfo;
		// for (u32 i = 0; i < sizeof(pciinfo); i += 4) {
			// *(reinterpret_cast<u32 *>(&pciinfo) + (i / 4)) = read_config_long(bus, slot, fn, i);
		// }
		//
		const char *x[] = {
			filename,
			rasprintf("%d", getProcessId()),
			rasprintf("%d", bus),
			rasprintf("%d", slot),
			rasprintf("%d", fn),
			0
		};

		pid_t r = bootstrap(filename, x);
		add_auth(r, vendor, device);

		delete [] x[0];
		delete [] x[1];
		delete [] x[2];
		delete [] x[3];

		// printf("\tbar0: %x, bar1: %x, bar2: %x, bar3: %x, bar4: %x, bar5: %x\n",
				// pciinfo.bar0, pciinfo.bar1, pciinfo.bar2,
				// pciinfo.bar3, pciinfo.bar4, pciinfo.bar5);
	}

	delete [] filename;
}

u16 read_config_word(u16 bus, u8 slot, u8 fn, u16 offset) {
	pci_config_cycle pcc = { 0, (offset & 0xFC) >> 2, fn, slot, bus, 0, 1 };
	AkariOutL(CONFIG_ADDRESS, *(reinterpret_cast<u32 *>(&pcc)));
	return ((AkariInL(CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
}

u32 read_config_long(u16 bus, u8 slot, u8 fn, u16 offset) {
	return read_config_word(bus, slot, fn, offset) | (read_config_word(bus, slot, fn, offset + 2) << 16);
}

void add_auth(pid_t pid, u16 vendor, u16 device) {
	u32 vendor_device = VENDOR_DEVICE(vendor, device);
	auth_map::iterator it = auths.find(pid);
	if (it == auths.end()) {
		auths[pid] = device_list_t();
		it = auths.find(pid);
	}

	if (std::find(it->second.begin(), it->second.end(), vendor_device) == it->second.end())
		it->second.push_back(vendor_device);
}

bool is_authed(pid_t pid, u16 vendor, u16 device) {
	u32 vendor_device = VENDOR_DEVICE(vendor, device);
	auth_map::iterator it = auths.find(pid);
	if (it == auths.end()) {
		return false;
	}

	return std::find(it->second.begin(), it->second.end(), vendor_device) != it->second.end();
}

