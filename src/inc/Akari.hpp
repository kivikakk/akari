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

#ifndef __AKARI_HPP__
#define __AKARI_HPP__

#include <arch.hpp>

class Memory;
class Console;
class Descriptor;
class Timer;
class Tasks;
class Syscall;
class ELF;
class Debugger;

extern Memory *mu_memory;
extern Console *mu_console;
extern Descriptor *mu_descriptor;
extern Timer *mu_timer;
extern Tasks *mu_tasks;
extern Syscall *mu_syscall;
extern ELF *mu_elf;
extern Debugger *mu_debugger;

#endif

