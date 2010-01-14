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
#include <List.hpp>

class Subsystem;
class Memory;
class Console;
class Descriptor;
class Timer;
class Tasks;
class Syscall;
class ELF;

/**
 * The base class for the kernel services.
 */
class Kernel {
	public:
		static Kernel *Construct(u32, u32);

		LinkedList<Subsystem *> subsystems;

		Memory *memory;
		Console *console;
		Descriptor *descriptor;
		Timer *timer;
		Tasks *tasks;
		Syscall *syscall;
		ELF *elf;
	
	protected:
		Kernel();
};

extern Kernel *Akari;

#endif

