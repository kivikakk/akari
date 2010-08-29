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

extern "C" int main(int argc, char **argv) {
	process_info_t *plist;

	int n = getProcessList(&plist);
	for (int j = 0; j < n; ++j) {
		process_info_t &i = plist[j];

		printf("% 6d  %d ", i.pid, i.cpl);

		printf(i.flags & PROCESS_FLAG_BLOCKING ? "B" : "-");
		printf(i.flags & PROCESS_FLAG_IRQ_LISTEN ? "Q" : "-");
		printf(i.flags & PROCESS_FLAG_CURRENT ? "R" : "-");

		printf(" ");

		if (i.registeredName) {
			printf("[%s] ", i.registeredName);
			int padding = 21 - 3 - strlen(i.registeredName);
			while (padding-- > 0)
				printf(" ");
			// maximal 21
		} else printf("                     ");

		if (i.name) {
			printf("%s", i.name);
		}

		printf("\n");
	}

	return 0;
}

