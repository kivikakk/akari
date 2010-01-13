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

static char *getline(u32 in) {
	u32 cs = 8, n = 0;
	char *kbbuf = static_cast<char *>(syscall_malloc(cs));

	while (true) {
		u32 incoming = syscall_readStream("system.io.keyboard", "input", in, kbbuf + n, 1);
		syscall_putc(kbbuf[n]);
		if (kbbuf[n] == '\n') break;

		n += incoming;	// 1

		if (n == cs) {
			char *nkb = static_cast<char *>(syscall_malloc(cs * 2));
			syscall_memcpy(nkb, kbbuf, n);
			syscall_free(kbbuf);
			kbbuf = nkb;
			cs *= 2;
		}
	}

	kbbuf[n] = 0;
	return kbbuf;
}

int strlen(const char *s) {
	int i = 0;
	while (*s)
		++i, ++s;
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


void ShellProcess() {
	u32 stdin = static_cast<u32>(-1);
	while (stdin == static_cast<u32>(-1)) {
		stdin = syscall_obtainStreamListener("system.io.keyboard", "input");
	}

	syscall_puts("\nstdin is 0x");
	syscall_putl(stdin, 16);
	syscall_puts(".\n");

	while (true) {
		char *l = getline(stdin);
		int s = strpos(l, " ");
		syscall_puts("space at ");
		syscall_putl(s, 10);
		syscall_puts("\n");
		syscall_free(l);

		// syscall_sendQueue("system.io.ata", 0, "hello", 6);
	}

	syscall_exit();
}

