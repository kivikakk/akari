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

#include <UserCalls.hpp>
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>

#include "../MBR/MBRProto.hpp"

pid_t keyboard_pid = 0;
u32 stdin = -1;

static char *getline() {
	u32 cs = 8, n = 0;
	char *kbbuf = new char[cs];

	while (true) {
		u32 incoming = syscall_readStream(keyboard_pid, "input", stdin, kbbuf + n, 1);
		syscall_putc(kbbuf[n]);
		if (kbbuf[n] == '\n') break;

		n += incoming;	// 1

		if (n == cs) {
			char *nkb = new char[cs * 2];
			syscall_memcpy(nkb, kbbuf, n);
			delete [] kbbuf;
			kbbuf = nkb;
			cs *= 2;
		}
	}

	kbbuf[n] = 0;
	return kbbuf;
}

int strlen(const char *s) {
	int i = 0;
	while (*s) ++i, ++s;
	return i;
}

int strcmpn(const char *s1, const char *s2, int n) {
	while (*s1 && *s2 && n > 0) {
		if (*s1 < *s2) return -1;
		if (*s1 > *s2) return 1;
		++s1, ++s2;
		--n;
	}
	if (n == 0) return 0;
	if (*s1 < *s2) return -1;
	if (*s1 > *s2) return 1;
	return 0;
}

int strpos(const char *haystack, const char *needle) {
	int i = 0;
	int hl = strlen(haystack), nl = strlen(needle);
	int d = hl - nl;
	while (i <= d) {
		if (strcmpn(haystack, needle, nl) == 0)
			return i;
		++i, ++haystack;
	}
	return -1;
}


extern "C" int start() {
	while (!keyboard_pid)
		keyboard_pid = syscall_processIdByName("system.io.keyboard");

	while (stdin == (u32)-1)
		stdin = syscall_obtainStreamListener(keyboard_pid, "input");

	syscall_puts("\nstdin is 0x");
	syscall_putl(stdin, 16);
	syscall_puts(".\n");

	while (true) {
		char *l = getline();
		int s = strpos(l, " ");
		syscall_puts("space at ");
		syscall_putl(s, 10);
		syscall_puts("\n\n");
		delete [] l;

		// Okay, let's grab the first 512 bytes.
		l = new char[512];

		MBROpRead op = {
			MBR_OP_READ,
			0,	// partition
			0,	// sector
			0,	// offset
			0x200	// lenght
		};

		u32 msg_id = syscall_sendQueue(syscall_processIdByName("system.io.mbr"), 0, reinterpret_cast<char *>(&op), sizeof(MBROpRead));
		syscall_puts("sent request #"); syscall_putl(msg_id, 16); syscall_putc('\n');

		struct queue_item_info *info = syscall_probeQueueFor(msg_id);
		syscall_puts("received reply #"); syscall_putl(info->id, 16);
		syscall_puts(" to #"); syscall_putl(info->reply_to, 16);
		syscall_puts(", length "); syscall_putl(info->data_len, 16); syscall_putc('\n');

		if (info->data_len != 512) syscall_panic("not 512 bytes back?");
		syscall_readQueue(info, l, 0, info->data_len);
		syscall_shiftQueue(info);

		syscall_puts("data follows:\n");
		for (int i = 0; i < 512; ++i)
			syscall_putc(l[i]);
		syscall_putc('\n');
		delete [] l;
	}

	syscall_panic("shell exited?");
	return 1;
}

