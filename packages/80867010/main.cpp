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
#include <list>

#include "main.hpp"
#include "../PCI/PCIProto.hpp"
#include "../ATA/ATAProto.hpp"

pid_t pci = 0, ata = 0;

extern "C" int main(int argc, char **argv) {
	pci = processIdByNameBlock("system.bus.pci");

	pci_device_regular *configs;

	{
		PCIOpDeviceConfig op = { PCI_OP_DEVICE_CONFIG };
		u32 msg_id = sendQueue(pci, 0, reinterpret_cast<u8 *>(&op), sizeof(PCIOpDeviceConfig));

		struct queue_item_info *info = probeQueueFor(msg_id);

		printf("80867010: msg id %d, %d config(s) (%d bytes leftover)\n",
				info->id,
				info->data_len / sizeof(pci_device_regular),
				info->data_len % sizeof(pci_device_regular));

		if (!info->data_len || info->data_len % sizeof(pci_device_regular)) {
			return 1;
		}

		if (info->data_len / sizeof(pci_device_regular) > 1) {
			printf("80867010: dunno what to do with many?\n");
			return 1;
		}

		configs = new pci_device_regular[info->data_len / sizeof(pci_device_regular)];
		readQueue(info, reinterpret_cast<u8 *>(configs), 0, info->data_len);
		shiftQueue(info);
	}

	ata = processIdByNameBlock("system.io.ata");
	
	{
		ATAOpSetBAR4 op = { ATA_OP_SET_BAR4, configs[0].bar4 & ~3 };
		sendQueue(ata, 0, reinterpret_cast<u8 *>(&op), sizeof(ATAOpSetBAR4));
	}

	printf("[80867010]\n");

	return 0;
}

