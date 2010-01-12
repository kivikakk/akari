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

#ifndef __DEBUG_HPP__
#define __DEBUG_HPP__

#ifdef DEBUG
#define ASSERT_STRINGIFY(x)	#x
#define ASSERT_TOSTRING(x)	ASSERT_STRINGIFY(x)
#define ASSERT(x) 	do { if (!(x)) { AkariPanic("Assertion at " __FILE__ ":" ASSERT_TOSTRING(__LINE__) " failed: " #x); } } while(0)
#else
#define ASSERT(x)	
#endif

extern "C" __attribute__((noreturn)) void AkariPanic(const char *);
extern "C" __attribute__((noreturn)) void AkariHalt();

#endif

