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
#include <stdarg.hpp>
#include <string>

static pid_t keyboard_pid = 0;
static u32 keyboard_hook = -1;

std::string getline() {
	while (!keyboard_pid)
		keyboard_pid = processIdByName("system.io.keyboard");

	while (keyboard_hook == (u32)-1)
		keyboard_hook = obtainStreamListener(keyboard_pid, "input");

	u32 cs = 8, n = 0;
	char *kbbuf = new char[cs];

	while (true) {
		u32 incoming = readStream(keyboard_pid, "input", keyboard_hook, kbbuf + n, 1);
		putc(kbbuf[n]);
		if (kbbuf[n] == '\n') break;

		n += incoming;	// 1

		if (n == cs) {
			cs *= 2;

			char *new_buf = new char[cs];
			memcpy(new_buf, kbbuf, n);
			delete [] kbbuf;
			kbbuf = new_buf;
		}
	}

	return std::string(kbbuf, n);
}

void printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);

	bool is_escape = false;
	char c;
	while ((c = *format++)) {
		if (c == '%') {
			if (!is_escape) {
				is_escape = true;
				continue;
			} else {
				putc(c);
				is_escape = false;
			}
		} else if (is_escape) {
			switch (c) {
			case 's':
				puts(va_arg(ap, const char *));
				break;
			case 'd':
				putl(va_arg(ap, u32), 10);
				break;
			case 'x':
				putl(va_arg(ap, u32), 16);
				break;
			}
			is_escape = false;
		} else {
			putc(c);
		}
	}

	va_end(ap);
}

