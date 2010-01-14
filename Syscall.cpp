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

#include <Syscall.hpp>
#include <Akari.hpp>
#include <UserCalls.hpp>
#include <UserIPC.hpp>
#include <Descriptor.hpp>

Syscall::Syscall(): _syscalls_assigned(0) {
	Akari->descriptor->idt->installHandler(0x80, &Syscall::_handler);

	addSyscall(0, reinterpret_cast<void *>(&User::putc));
	addSyscall(1, reinterpret_cast<void *>(&User::puts));
	addSyscall(2, reinterpret_cast<void *>(&User::putl));
	addSyscall(3, reinterpret_cast<void *>(&User::getProcessId));
	addSyscall(4, reinterpret_cast<void *>(&User::irqWait));
	addSyscall(5, reinterpret_cast<void *>(&User::irqListen));
	addSyscall(6, reinterpret_cast<void *>(&User::panic));
	addSyscall(7, reinterpret_cast<void *>(&User::exit));
	addSyscall(8, reinterpret_cast<void *>(&User::defer));
	addSyscall(9, reinterpret_cast<void *>(&User::malloc));
	addSyscall(10, reinterpret_cast<void *>(&User::free));
	addSyscall(11, reinterpret_cast<void *>(&User::memcpy));

	addSyscall(24, reinterpret_cast<void *>(&User::IPC::processId));
	addSyscall(25, reinterpret_cast<void *>(&User::IPC::processIdByName));
	addSyscall(12, reinterpret_cast<void *>(&User::IPC::registerName));

	addSyscall(13, reinterpret_cast<void *>(&User::IPC::registerStream));
	addSyscall(14, reinterpret_cast<void *>(&User::IPC::obtainStreamWriter));
	addSyscall(15, reinterpret_cast<void *>(&User::IPC::obtainStreamListener));
	addSyscall(16, reinterpret_cast<void *>(&User::IPC::readStream));
	addSyscall(17, reinterpret_cast<void *>(&User::IPC::readStreamUnblock));
	addSyscall(18, reinterpret_cast<void *>(&User::IPC::writeStream));

	addSyscall(19, reinterpret_cast<void *>(&User::IPC::probeQueue));
	addSyscall(20, reinterpret_cast<void *>(&User::IPC::probeQueueUnblock));
	addSyscall(26, reinterpret_cast<void *>(&User::IPC::probeQueueFor));
	addSyscall(27, reinterpret_cast<void *>(&User::IPC::probeQueueForUnblock));
	addSyscall(21, reinterpret_cast<void *>(&User::IPC::readQueue));
	addSyscall(22, reinterpret_cast<void *>(&User::IPC::shiftQueue));
	addSyscall(23, reinterpret_cast<void *>(&User::IPC::sendQueue));
}

u8 Syscall::versionMajor() const { return 0; }
u8 Syscall::versionMinor() const { return 1; }
const char *Syscall::versionManufacturer() const { return "Akari"; }
const char *Syscall::versionProduct() const { return "Akari Syscall"; }

void Syscall::addSyscall(u16 num, void *fn) {
	_syscalls[num] = fn;
}

void Syscall::returnToTask(Tasks::Task *task) {
	_returnTask = task;
}

void Syscall::returnToNextTask() {
	Tasks::Task *nextTask = Akari->tasks->prepareFetchNextTask();
	if (nextTask == Akari->tasks->current) {
		AkariPanic("TODO: let no 'active' processes being running. i.e. have the ukernel HLT or similar. -- bigger question; why isn't idle task running!? or why is it trying to 'returnToNextTask' from a syscall?");
	}
		
	returnToTask(nextTask);
}

void *Syscall::_handler(struct modeswitch_registers *regs) {
	if (regs->callback.eax >= AKARI_SYSCALL_MAXCALLS)
		AkariPanic("System call greater than maximum requested. TODO: kill requesting process.");
	if (!Akari->syscall->_syscalls[regs->callback.eax])
		AkariPanic("Non-existing system call requested.");

	Akari->syscall->_returnTask = 0;

	void *call = Akari->syscall->_syscalls[regs->callback.eax];
    int ret;
    asm volatile("  \
        push %1; \
        push %2; \
        push %3; \
        push %4; \
        push %5; \
        call *%6; \
        pop %%ebx; \
        pop %%ebx; \
        pop %%ebx; \
        pop %%ebx; \
        pop %%ebx; \
        "
            : "=a" (ret)
            : "r" (regs->callback.edi), "r" (regs->callback.esi), "r" (regs->callback.edx), "r" (regs->callback.ecx), "r" (regs->callback.ebx), "r" (call)
            : "%ebx");

    regs->callback.eax = ret;

	if (Akari->syscall->_returnTask) {
		Akari->tasks->saveRegisterToTask(Akari->tasks->current, regs);
		Akari->tasks->current = Akari->syscall->_returnTask;
		return Akari->tasks->assignInternalTask(Akari->tasks->current);
	}

	return regs;
}
