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

#define DEVICE(bus, slot, fn) \
	((u32)((u16)(bus)) << 16) | \
	((u32)((u8)(slot)) << 8) | \
	(u32)((u8)(fn))

#define D_BUS(device)	(u16)((device) >> 16)
#define D_SLOT(device)	(u8)((device) >> 8)
#define D_FN(device)	(u8)(device)

typedef u32 device_t;
typedef std::list<device_t> device_list_t;
typedef std::map<pid_t, device_list_t> auth_map;
static auth_map auths;

static void check_all_devices(bool hds);
static bool init();
static u16 check_vendor(u16 bus, u8 slot, u8 fn);
static void check_device(u16 bus, u8 slot, u8 fn, u16 vendor, u16 device);
static u16 read_config_word(u16 bus, u8 slot, u8 fn, u16 offset);
static u32 read_config_long(u16 bus, u8 slot, u8 fn, u16 offset);
static void add_auth(pid_t pid, u16 bus, u8 slot, u8 fn);
static device_list_t &authed(pid_t pid);

static bool non_hds_brought_up = false;

extern "C" int main() {
	if (!init()) {
		printf("PCI: failed init\n");
		return 1;
	}
	if (!registerName("system.bus.pci"))
		panic("PCI: could not register system.bus.pci");

	printf("[PCI]\n");

	while (true) {
		struct queue_item_info info = *probeQueue();
		u8 *request = grabQueue(&info);
		shiftQueue(&info);

		if (request[0] == PCI_OP_DEVICE_CONFIG) {
			device_list_t &l = authed(info.from);

			pci_device_regular pciinfo[l.size()];

			u32 offset = 0;
			for (device_list_t::iterator it = l.begin(); it != l.end(); ++it) {
				for (u32 i = 0; i < sizeof(pciinfo); i += 4) {
					*reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(&pciinfo) + i + offset) =
						read_config_long(D_BUS(*it), D_SLOT(*it), D_FN(*it), i);
				}
				offset += sizeof(pci_device_regular);
			}

			sendQueue(info.from, info.id, reinterpret_cast<u8 *>(&pciinfo), offset);
		} else if (request[0] == PCI_OP_DMA_UP) {
			// Apparently we have DMA. Bring up other devices if we haven't already.

			if (!non_hds_brought_up) {
				non_hds_brought_up = true;
				check_all_devices(false);
			}	
		} else {
			panic("PCI: confused");
		}

		delete [] request;
	}

	panic("PCI: ran off the end of the infinite loop");
	return 1;
}

const u32 known_harddisk_drivers[] = {
	0x80867010,
	0
};

void check_all_devices(bool hds) {
	for (u16 bus = 0; bus < 256; ++bus) {
		for (u8 slot = 0; slot < 32; ++slot) {
			for (u8 fn = 0; fn < 8; ++fn) {
				u16 vendor = check_vendor(bus, slot, fn);

				if (fn == 0 && vendor == 0xFFFF) {
					break;
				} else if (vendor != 0xFFFF) {
					u16 device = read_config_word(bus, slot, fn, 2);

					bool is_hd = false;
					const u32 *p = known_harddisk_drivers;
					while (*p && !is_hd)
						if (*p++ == ((static_cast<u32>(vendor) << 16) | device))
							is_hd = true;

					if ((hds && is_hd) || (!hds && !is_hd)) {
						check_device(bus, slot, fn, vendor, device);
					}
				}
			}
		}
	}
}

bool init() {
	check_all_devices(true);

	return true;
}

u16 check_vendor(u16 bus, u8 slot, u8 fn) {
	return read_config_word(bus, slot, fn, 0);
}

void check_device(u16 bus, u8 slot, u8 fn, u16 vendor, u16 device) {
	printf("%x/%x/%x: vendor %x, device %x\n", bus, slot, fn, vendor, device);

	char *filename = rasprintf("/%4x%4x", vendor, device);

	if (fexists(filename)) {
		pid_t r = bootstrap(filename, 0);
		add_auth(r, bus, slot, fn);
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

void add_auth(pid_t pid, u16 bus, u8 slot, u8 fn) {
	device_t vendor_device = DEVICE(bus, slot, fn);

	auth_map::iterator it = auths.find(pid);
	if (it == auths.end()) {
		auths[pid] = device_list_t();
		it = auths.find(pid);
	}

	if (std::find(it->second.begin(), it->second.end(), vendor_device) == it->second.end())
		it->second.push_back(vendor_device);
}

device_list_t &authed(pid_t pid) {
	auth_map::iterator it = auths.find(pid);
	if (it == auths.end()) {
		auths[pid] = device_list_t();
		return auths[pid];
	}

	return it->second;
}

