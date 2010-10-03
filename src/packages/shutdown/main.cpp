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
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>

#include "main.hpp"
#include "../acpi/proto.hpp"

int usage() {
	printf("Usage: shutdown OPTION\n");
	printf("Bring the system down.\n\n");
	printf("Options:\n");
	printf("  -r                reboot after shutdown\n");
	printf("  -h                halt or power off after shutdown\n");
	return 0;
}

extern "C" int main(int argc, char **argv) {
	pid_t acpi = processIdByNameBlock("system.bus.acpi");

	if (argc < 2) return usage();

	bool reboot = false, halt = false, unknown = false;
	++argv;
	while (*argv) {
		if (strcmp(*argv, "-r") == 0)
			reboot = true;
		else if (strcmp(*argv, "-h") == 0)
			halt = true;
		else
			unknown = true;
		++argv;
	}

	if ((reboot && halt) || unknown) return usage();

	if (halt) {
		ACPIOpShutdown op = { ACPI_OP_SHUTDOWN };
		sendQueue(acpi, 0, reinterpret_cast<u8 *>(&op), sizeof(ACPIOpShutdown));
	} else if (reboot) {
		ACPIOpReboot op = { ACPI_OP_REBOOT };
		sendQueue(acpi, 0, reinterpret_cast<u8 *>(&op), sizeof(ACPIOpReboot));
	}

	return 0;
}

