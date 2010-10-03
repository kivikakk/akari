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

#include "../pci/proto.hpp"

extern "C" int main() {
	// ACPI
	bootstrap_options_t bsops;

	// Give access to first mem page (for 0x40E for EBDA),
	// and 0x000E0000 ~ 0x00100000.
	bsops.mmap[0x0000] = 0x0000;
	for (u32 i = 0xE0000; i < 0x100000; i += 0x1000)
		bsops.mmap[i] = i;
	bsops.privs.push_back(PRIV_PHYSADDR);
	bsops.privs.push_back(PRIV_GRANT_PRIV);
	bsops.privs.push_back(PRIV_POWER_MGMT);

	pid_t acpi = bootstrap("/sys/services/acpi", std::slist<std::string>(), bsops);
	registerName(acpi, "system.bus.acpi");

	// PCI
	bsops = bootstrap_options_t();

	bsops.iobits.push_back(0xCF8);
	bsops.iobits.push_back(0xCF9);
	bsops.iobits.push_back(0xCFA);
	bsops.iobits.push_back(0xCFB);
	bsops.iobits.push_back(0xCFC);
	bsops.iobits.push_back(0xCFD);
	bsops.iobits.push_back(0xCFE);
	bsops.iobits.push_back(0xCFF);
	bsops.privs.push_back(PRIV_GRANT_PRIV);

	pid_t pci = bootstrap("/sys/services/pci", std::slist<std::string>(), bsops);
	registerName(pci, "system.bus.pci");
	
	{
		PCIOpAwaitDriversUp op = { PCI_OP_AWAIT_DRIVERS_UP };
		u32 msg_id = sendQueue(pci, 0, reinterpret_cast<u8 *>(&op), sizeof(PCIOpAwaitDriversUp));
		shiftQueue(probeQueueFor(msg_id));
	}

	// Keyboard
	bsops = bootstrap_options_t();

	bsops.iobits.push_back(0x60);
	bsops.iobits.push_back(0x64);

	bsops.privs.push_back(PRIV_IRQ);

	pid_t keyboard = bootstrap("/sys/services/keyboard", std::slist<std::string>(), bsops);
	registerName(keyboard, "system.io.keyboard");

	// sh
	bootstrap("/bin/sh");

	return 0;
}

