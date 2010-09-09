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
	printf("Booter: started\n");

	bootstrap_options_t bsops;

	bsops.iobits.push_back(0xCF8);
	bsops.iobits.push_back(0xCF9);
	bsops.iobits.push_back(0xCFA);
	bsops.iobits.push_back(0xCFB);
	bsops.iobits.push_back(0xCFC);
	bsops.iobits.push_back(0xCFD);
	bsops.iobits.push_back(0xCFE);
	bsops.iobits.push_back(0xCFF);

	pid_t pci = bootstrap("/PCI", std::slist<std::string>(), bsops);
	registerName(pci, "system.bus.pci");
	
	{
		PCIOpAwaitDriversUp op = { PCI_OP_AWAIT_DRIVERS_UP };
		u32 msg_id = sendQueue(pci, 0, reinterpret_cast<u8 *>(&op), sizeof(PCIOpAwaitDriversUp));
		shiftQueue(probeQueueFor(msg_id));
	}

	bsops.iobits.clear();
	bsops.iobits.push_back(0x60);
	bsops.iobits.push_back(0x64);
	bsops.privs.push_back(PRIV_IRQ);

	pid_t keyboard = bootstrap("/Keyboard", std::slist<std::string>(), bsops);
	registerName(keyboard, "system.io.keyboard");

	bootstrap("/Shell");

	return 0;
}

