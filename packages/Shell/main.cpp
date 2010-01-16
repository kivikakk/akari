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

extern "C" int start() {
	while (!keyboard_pid)
		keyboard_pid = processIdByName("system.io.keyboard");

	while (stdin == (u32)-1)
		stdin = obtainStreamListener(keyboard_pid, "input");

	while (true) {
		char *l = getline();
		// int s = strpos(l, " ");
		// printf("space at %d\n\n", s);
		delete [] l;

		// Okay, let's grab the first 512 bytes of something.
		l = new char[512];

		DIR *dirp = opendir("/");
		printf("Shell: dirp is %x, dirp->dir is %x\n", dirp, dirp->dir);
		VFSDirent *dirent;
		while ((dirent = readdir(dirp))) {
			printf("Shell: name is %s, ino is %x - going to give it a go\n", dirent->name, dirent->inode);

			char *name = new char[strlen(dirent->name) + 2];
			*name = '/';
			strcpy(name + 1, dirent->name);

			FILE *file = fopen(name, "r");
			printf("Shell:\tgot file handle for %s: %x\n", name, file);
			printf("Shell:\tlen is %x\n", flen(file));

			u8 *buf = new u8[flen(file) + 1];
			fread(buf, flen(file), 1, file);
			buf[flen(file)] = 0;

			printf("%s\n", buf);

			fclose(file);

			delete [] getline();
		}
		closedir(dirp);
	}

	panic("shell exited?");
	return 1;
}

