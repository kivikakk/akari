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
#include <stdarg.hpp>

// XXX This isn't really the place for it, but I can't help!
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

