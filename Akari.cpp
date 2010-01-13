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

#include <Akari.hpp>
#include <Memory.hpp>
#include <Console.hpp>
#include <Descriptor.hpp>
#include <debug.hpp>

Kernel *Akari;

Kernel::Kernel(): memory(0), console(0), descriptor(0), timer(0), tasks(0), syscall(0) {
}

/**
 * Constructs an Kernel, using the given address as the current placement address.
 */
Kernel *Kernel::Construct(u32 addr, u32 upperMemory) {
	Kernel *kernel = new (reinterpret_cast<void *>(addr)) Kernel();
	addr += sizeof(Kernel);
	kernel->memory = new (reinterpret_cast<void *>(addr)) Memory(upperMemory);
	addr += sizeof(Memory);

	kernel->memory->setPlacementMode(addr);

	return kernel;
}
