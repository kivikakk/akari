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
#include <list>

// HACK: used in conjunction with __cxa_atexit, we're not doing anything dynamic, so just ignore it...
void *__dso_handle = (void *)&__dso_handle;

static std::list<void (*)(void *)> exit_handlers;

extern "C" void __cxa_pure_virtual() {
	panic("__cxa_pure_virtual called in usermode");
}

extern "C" int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle) {
	if (arg || dso_handle)
		panic("__cxa_atexit doesn't know how to use arg or dso_handle");

	exit_handlers.push_back(func);
	return 0;
}

extern "C" void exit(int status) {
	for (std::list<void (*)(void *)>::reverse_iterator it = exit_handlers.rbegin(); it != exit_handlers.rend(); ++it)
		(*it)(0);

	if (status)
		panic("non-zero exit from process");
	sysexit();
}

extern "C" int main();
extern "C" void start() {
	exit(main());
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

