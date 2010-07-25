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

#include "test_helper.hpp"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

Kernel *Akari = new Kernel();

Kernel::Kernel(): console(new Console())
{ }

void Console::printf(const char *s, ...) {
	va_list ap;
	va_start(ap, s);
	vprintf(s, ap);
	va_end(ap);
}

void *memset(void *s, int c, int n) {
	char *w = (char *)s;
	while (n--)
		*w = c;
	return s;
}

void AkariPanic(const char *s) {
	fprintf(stderr, "PANIC: %s\n", s);
	exit(1);
}
