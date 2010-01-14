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

#ifndef __POSIX_HPP__
#define __POSIX_HPP__

#include <arch.hpp>

namespace POSIX {
	extern "C" void *memset(void *, u8, u32);
	extern "C" void *memcpy(void *, const void *, u32);

	extern "C" u32 strlen(const char *);
	extern "C" s32 strcmp(const char *s1, const char *s2);
	extern "C" char *strcpy(char *dest, const char *src);
	extern "C" char *strdup(const char *src);
}

#endif

