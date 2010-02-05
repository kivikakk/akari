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

#include <UserProcess.hpp>

#if defined(__AKARI_KERNEL)

#include <debug.hpp>
#include <Tasks.hpp>
#include <ELF.hpp>
#include <Akari.hpp>

namespace User {
namespace Process {
	pid_t fork() {
		// Copy ourselves and set us running!
		AkariPanic("The more I reflect on it, the more I realise fork is really challenging. TODO.");
		return 0;
	}

	pid_t spawn(const char *name, const u8 *elf, u32 elf_len) {
		Tasks::Task *new_task = Tasks::Task::CreateTask(0, 3, true, 0, Akari->memory->_kernelDirectory, name);

		Akari->elf->loadImageInto(new_task, elf);
		
		// Oh, this is terrible. XXX
		new_task->next = Akari->tasks->start;
		Akari->tasks->start = new_task;

		// HACK for KB to work here.
		new_task->setIOMap(0x60, true);
		new_task->setIOMap(0x64, true);

		// HACK for PCI to work
		new_task->setIOMap(0xCF8, true);
		new_task->setIOMap(0xCF9, true);
		new_task->setIOMap(0xCFA, true);
		new_task->setIOMap(0xCFB, true);
		new_task->setIOMap(0xCFC, true);
		new_task->setIOMap(0xCFD, true);
		new_task->setIOMap(0xCFE, true);
		new_task->setIOMap(0xCFF, true);

		return new_task->id;
	}
}
}
#endif
