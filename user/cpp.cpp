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
#include <cpp.hpp>

extern "C" void __cxa_pure_virtual() {
	panic("__cxa_pure_virtual called in usermode");
}

void *operator new(size_t n) {
	return malloc(n);
}

void *operator new[](size_t n) {
	return malloc(n);
}

void *operator new(size_t, void *p) {
	return p;
}

void operator delete(void *p) {
	return free(p);
}

void operator delete[](void *p) {
	return free(p);
}

