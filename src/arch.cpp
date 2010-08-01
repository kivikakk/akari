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

#include <arch.hpp>
#include <debug.hpp>
#include <Akari.hpp>
#include <Memory.hpp>

void __cxa_pure_virtual() {
	AkariPanic("__cxa_pure_virtual called");
}

void *operator new(size_t n) {
	ASSERT(Akari);
	ASSERT(Akari->memory);
	return Akari->memory->alloc(n);
}

void *operator new[](size_t n) {
	ASSERT(Akari);
	ASSERT(Akari->memory);
	return Akari->memory->alloc(n);
}

void *operator new(size_t, void *p) {
	return p;
}

void operator delete(void *p) {
	ASSERT(Akari);
	ASSERT(Akari->memory);
	Akari->memory->free(p);
}

void operator delete[](void *p) {
	// These assertions seem quite wasteful.
	ASSERT(Akari);
	ASSERT(Akari->memory);
	Akari->memory->free(p);
}
