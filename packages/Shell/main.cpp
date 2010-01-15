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

#include "../VFS/VFSProto.hpp"

pid_t keyboard_pid = 0;
u32 stdin = -1;

static char *getline() {
	u32 cs = 8, n = 0;
	char *kbbuf = new char[cs];

	while (true) {
		u32 incoming = readStream(keyboard_pid, "input", stdin, kbbuf + n, 1);
		putc(kbbuf[n]);
		if (kbbuf[n] == '\n') break;

		n += incoming;	// 1

		if (n == cs) {
			char *nkb = new char[cs * 2];
			memcpy(nkb, kbbuf, n);
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
		keyboard_pid = processIdByName("system.io.keyboard");

	while (stdin == (u32)-1)
		stdin = obtainStreamListener(keyboard_pid, "input");

	while (true) {
		char *l = getline();
		int s = strpos(l, " ");
		printf("space at %d\n\n", s);
		delete [] l;

		// Okay, let's grab the first 512 bytes of something.
		l = new char[512];

		int i = 0;
		while (true) {
			VFSOpReaddir op = {
				VFS_OP_READDIR,
				0,
				i
			};

			u32 msg_id = sendQueue(processIdByName("system.io.vfs"), 0, reinterpret_cast<u8 *>(&op), sizeof(op));

			struct queue_item_info *info = probeQueueFor(msg_id);
			if (info->data_len == 0) {
				printf("end of dirents (at %d)\n", i);
				shiftQueue(info);
				break;
			}

			VFSDirent dirent;
			readQueue(info, reinterpret_cast<u8 *>(&dirent), 0, info->data_len);
			shiftQueue(info);

			printf("dirent:\n");
			printf("\tname:  %s\n", dirent.name);
			printf("\tinode: %x\n\n", dirent.inode);

			i++;
		}
	}

	panic("shell exited?");
	return 1;
}

