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
#include <fs.hpp>
#include <UserCalls.hpp>
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>
#include <arch.hpp>
#include <proc.hpp>

#include "../PCI/PCIProto.hpp"

extern "C" int main() {
	pid_t pci = bootstrap("/PCI", 0);
	registerName(pci, "system.bus.pci");
	
	{
		PCIOpAwaitDriversUp op = { PCI_OP_AWAIT_DRIVERS_UP };
		u32 msg_id = sendQueue(pci, 0, reinterpret_cast<u8 *>(&op), sizeof(PCIOpAwaitDriversUp));
		shiftQueue(probeQueueFor(msg_id));
	}

	printf("Booter: started\n");

	pid_t kb = bootstrap("/Keyboard", 0);
	grantPrivilege(kb, PRIV_IRQ);
	registerName(kb, "system.io.keyboard");

	bootstrap("/Shell", 0);

	return 0;
}

