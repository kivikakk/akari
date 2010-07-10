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

#ifndef __STDARG_HPP__
#define __STDARG_HPP__

// This is probably GCC specific.

typedef struct {
	u32 *ptr;
} va_list;

#define va_start(ap, place) \
	ap.ptr = (u32 *)&place

#define va_arg(ap, type) \
	((type)*(++(ap).ptr))

#define va_end(ap) \
	ap.ptr = (u32 *)0

#endif

