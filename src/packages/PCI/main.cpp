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
#include <slist>
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

typedef u32 device_t, venddev_t;
typedef std::map< venddev_t, std::slist<device_t> > device_list_t;

void check_hds();
void check_non_hds();
static void check_all_devices();
static bool init();
static u16 check_vendor(u16 bus, u8 slot, u8 fn);
static u16 read_config_word(u16 bus, u8 slot, u8 fn, u16 offset);
static u32 read_config_long(u16 bus, u8 slot, u8 fn, u16 offset);

static bool non_hds_brought_up = false;

typedef struct {
	pid_t pid;
	u32 msg_id;
} AwaitingDriversUp;

static std::slist<AwaitingDriversUp> awaiting_drivers_up;
static device_list_t all_devices;

extern "C" int main() {
	if (!init()) {
		printf("PCI: failed init\n");
		return 1;
	}

	printf("PCI: started\n");

	while (true) {
		struct queue_item_info info = *probeQueue();
		u8 *request = grabQueue(&info);
		shiftQueue(&info);

		if (request[0] == PCI_OP_DEVICE_CONFIG) {
			PCIOpDeviceConfig *op = reinterpret_cast<PCIOpDeviceConfig *>(request);

			std::slist<device_t> &dl = all_devices[((u32)op->device << 16) | op->vendor];
			u32 length = dl.size(), offset = 0;
			pci_device_regular *pciinfo = new pci_device_regular[length];

			for (std::slist<device_t>::iterator it = dl.begin(); it != dl.end(); ++it) {
				for (u32 i = 0; i < sizeof(pci_device_regular); i += 4) {
					*reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(pciinfo) + i + offset) =
						read_config_long(D_BUS(*it), D_SLOT(*it), D_FN(*it), i);
				}
				offset += sizeof(pci_device_regular);
			}

			sendQueue(info.from, info.id, reinterpret_cast<u8 *>(pciinfo), offset);

			delete [] pciinfo;
		} else if (request[0] == PCI_OP_DMA_UP) {
			// Apparently we have DMA. Bring up other devices if we haven't already.

			if (!non_hds_brought_up) {
				non_hds_brought_up = true;
				check_non_hds();

				for (std::slist<AwaitingDriversUp>::iterator it = awaiting_drivers_up.begin(); it != awaiting_drivers_up.end(); ++it)
					sendQueue(it->pid, it->msg_id, 0, 0);

				awaiting_drivers_up.clear();
			}	
		} else if (request[0] == PCI_OP_AWAIT_DRIVERS_UP) {
			if (non_hds_brought_up) {
				sendQueue(info.from, info.id, 0, 0);
			} else {
				AwaitingDriversUp adu = { info.from, info.id };
				awaiting_drivers_up.push_back(adu);
			}
		} else {
			panic("PCI: confused");
		}

		delete [] request;
	}
}

typedef struct {
	const char *name;
	u16 vendor, device;
} name_vendor_pair_t;

const name_vendor_pair_t known_harddisk_drivers[] = {
	{ "PIIX3", 0x8086, 0x7010 },
	{ 0, 0, 0 }
};

void check_hds() {
	const name_vendor_pair_t *p = known_harddisk_drivers;
	while (p->name) {
		char *filename = rasprintf("/drivers/%s", p->name);
		if (fexists(filename)) {
			bootstrap(filename, 0);
		}

		delete [] filename;

		++p;
	}
}

void check_non_hds() {
	DIR *dirp = opendir("/drivers/");
	VFSDirent *dirent;

	while ((dirent = readdir(dirp))) {
		bool is_hd = false;

		const name_vendor_pair_t *p = known_harddisk_drivers;
		while (!is_hd && p->name) {
			if (stricmp(dirent->name, p->name) == 0) {
				is_hd = true;
			}
			++p;
		}

		if (is_hd) continue;
		printf("bootstrapping %s\n", dirent->name);
		//bootstrap(
	}

	closedir(dirp);
}

void check_all_devices() {
	for (u16 bus = 0; bus < 256; ++bus) {
		for (u8 slot = 0; slot < 32; ++slot) {
			for (u8 fn = 0; fn < 8; ++fn) {
				u16 vendor = check_vendor(bus, slot, fn);

				if (fn == 0 && vendor == 0xFFFF) {
					break;
				} else if (vendor != 0xFFFF) {
					u16 device = read_config_word(bus, slot, fn, 2);

					std::slist<device_t> &dl = all_devices[((u32)vendor) << 16 | (u32)device];
					dl.push_back(DEVICE(bus, slot, fn));
				}
			}
		}
	}
}

bool init() {
	check_hds();
	check_all_devices();

	return true;
}

u16 check_vendor(u16 bus, u8 slot, u8 fn) {
	return read_config_word(bus, slot, fn, 0);
}

u16 read_config_word(u16 bus, u8 slot, u8 fn, u16 offset) {
	pci_config_cycle pcc = { 0, (offset & 0xFC) >> 2, fn, slot, bus, 0, 1 };
	AkariOutL(CONFIG_ADDRESS, *(reinterpret_cast<u32 *>(&pcc)));
	return ((AkariInL(CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
}

u32 read_config_long(u16 bus, u8 slot, u8 fn, u16 offset) {
	return read_config_word(bus, slot, fn, offset) | (read_config_word(bus, slot, fn, offset + 2) << 16);
}

