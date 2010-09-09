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
#include <slist>
#include <vector>
#include <debug.hpp>

#include "RTL8139Proto.hpp"
#include "main.hpp"
#include "../PCI/PCIProto.hpp"

pid_t pci = 0;

extern "C" int main() {
	pci = processIdByNameBlock("system.bus.pci");

	pci_device_regular *configs;

	{
		PCIOpDeviceConfig op = { PCI_OP_DEVICE_CONFIG, 0x10EC, 0x8139 };
		u32 msg_id = sendQueue(pci, 0, reinterpret_cast<u8 *>(&op), sizeof(PCIOpDeviceConfig));

		struct queue_item_info *info = probeQueueFor(msg_id);

		printf("RTL8139: %d config(s) (%d bytes leftover)\n",
			info->data_len / sizeof(pci_device_regular),
			info->data_len % sizeof(pci_device_regular));

		if (!info->data_len || info->data_len % sizeof(pci_device_regular)) {
			return 1;
		}

		if (info->data_len / sizeof(pci_device_regular) > 1) {
			printf("RTL8139: dunno what to do with many?\n");
			return 1;
		}

		configs = new pci_device_regular[info->data_len / sizeof(pci_device_regular)];
		readQueue(info, reinterpret_cast<u8 *>(configs), 0, info->data_len);
		shiftQueue(info);
	}

	// Let's power it up.
	u16 iobase = static_cast<u16>(configs->bar0) & ~0x3;
	u8 irq = static_cast<u8>(configs->interrupt_line);
	u8 macaddr[] = {
		AkariInB(iobase),
		AkariInB(iobase + 1),
		AkariInB(iobase + 2),
		AkariInB(iobase + 3),
		AkariInB(iobase + 4),
		AkariInB(iobase + 5)
	};

	printf("RTL8139: started (IO base %4x, IRQ %d)\n", iobase, irq);
	printf("RTL8139: MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
			macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);

	AkariOutB(iobase + 0x52, 0x00);
	AkariOutB(iobase + 0x37, 0x10);
	while (AkariInB(iobase + 0x37) & 0x10) {
		printf(".");
	}

	phptr pptr;
	u8 *localbuff = static_cast<u8 *>(mallocap(8192 + 16, &pptr));
	AkariOutL(iobase + 0x30, pptr);
	AkariOutW(iobase + 0x3C, 0x0005);	// TOK, ROK
	AkariOutL(iobase + 0x44, 0xF);		// 1<<7 is WRAP bit. 0xF = AccBroad + AccMulti + AccPhysMatch + AccAllPack
	AkariOutB(iobase + 0x37, 0x0C);		// RE + TE high
	
	while (true) {
		struct queue_item_info info = *probeQueue();
		u8 *request = grabQueue(&info);
		shiftQueue(&info);

		if (request[0] == RTL8139_OP_NOOP) {
			// RTL8139OpNoop *op = reinterpret_cast<RTL8139OpNoop *>(request);

			// u8 *buffer = new u8[op->length];
			// sendQueue(info.from, info.id, buffer, bytes_read);
			// delete [] buffer;
		} else {
			panic("RTL8139: confused");
		}
	}
}

