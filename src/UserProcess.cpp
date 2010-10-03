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
#include <UserCalls.hpp>
#include <Tasks.hpp>
#include <ELF.hpp>
#include <Akari.hpp>
#include <Console.hpp>

namespace User {
namespace Process {
	pid_t fork() {
		// Copy ourselves and set us running!
		AkariPanic("The more I reflect on it, the more I realise fork is really challenging. TODO.");
		return 0;
	}

	pid_t spawn(const char *name, const u8 *elf, u32 elf_len, char *const *args) {
		std::list<std::string> argl;

		while (args && *args) 
			argl.push_back(*args++);

		Tasks::Task *new_task = Tasks::Task::CreateTask(0, 3, true, 0, mu_memory->_kernelDirectory, name, argl);
		mu_elf->loadImageInto(new_task, elf);
		return new_task->id;
	}

	bool mapPhysicalMem(pid_t taskpid, u32 virt_from, u32 virt_to, u32 phys_from, bool readwrite) {
		requirePrivilege(PRIV_GRANT_PRIV);
		requirePrivilege(PRIV_PHYSADDR);
		Tasks::Task *task = mu_tasks->tasksByPid[taskpid];
		if (!task)
			return false;
		Memory::Page *page;
		
		for (; virt_from <= virt_to; virt_from += 0x1000, phys_from += 0x1000) {
			page = task->pageDir->getPage(virt_from, true);
			page->present = true;
			page->readwrite = readwrite;
			page->user = true;
			page->pageAddress = phys_from / 0x1000;
		}

		return true;
	}

	bool grantPrivilege(pid_t taskpid, u16 priv) {
		requirePrivilege(PRIV_GRANT_PRIV);
		Tasks::Task *task = mu_tasks->tasksByPid[taskpid];
		if (!task)
			return false;
		task->grantPrivilege(static_cast<priv_t>(priv));
		return true;
	}

	bool grantIOPriv(pid_t taskpid, u16 port) {
		requirePrivilege(PRIV_GRANT_PRIV);
		Tasks::Task *task = mu_tasks->tasksByPid[taskpid];
		if (!task)
			return false;
		task->setIOMap(port, true);
		return true;
	}

	bool beginExecution(pid_t taskpid) {
		Tasks::Task *task = mu_tasks->tasksByPid[taskpid];
		if (!task)
			return false;
		task->next = mu_tasks->start;
		mu_tasks->start = task;
		return true;
	}

	int nanosleep(const struct timespec *req, struct timespec *rem) {
		if (rem) panic("Whoops! 'rem' is ignored in nanosleep.");

		panic("Not implemented. Eek.");
		return -1;
	}
}
}
#endif
