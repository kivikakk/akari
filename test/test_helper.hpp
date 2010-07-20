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

#ifndef __TEST_HELPER_HPP__
#define __TEST_HELPER_HPP__

#define DEBUG
#define __PTR_TYPE_DEFINED__
typedef unsigned long long ptr_t;

extern "C" void *memset(void *s, int c, int n);
extern "C" void AkariPanic(const char *s);

#endif
