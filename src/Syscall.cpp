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
#include <UserIPCQueue.hpp>
#include <UserProcess.hpp>
#include <Descriptor.hpp>

Syscall::Syscall(): _syscalls_assigned(0), _returnTask(0) {
	mu_descriptor->idt->installHandler(0x80, &Syscall::_handler);

	// TODO: renumber me someday. There are big holes.
	addSyscall(0, reinterpret_cast<syscall_fn_t>(&User::putc));
	addSyscall(1, reinterpret_cast<syscall_fn_t>(&User::puts));
	addSyscall(3, reinterpret_cast<syscall_fn_t>(&User::getProcessId));
	addSyscall(4, reinterpret_cast<syscall_fn_t>(&User::irqWait));
	addSyscall(38, reinterpret_cast<syscall_fn_t>(&User::irqWaitTimeout));
	addSyscall(5, reinterpret_cast<syscall_fn_t>(&User::irqListen));
	addSyscall(37, reinterpret_cast<syscall_fn_t>(&User::ticks));
	addSyscall(6, reinterpret_cast<syscall_fn_t>(&User::panic));
	addSyscall(7, reinterpret_cast<syscall_fn_t>(&User::sysexit));		// Don't change my number from 7 without changing entry.cpp.
	addSyscall(8, reinterpret_cast<syscall_fn_t>(&User::defer));
	addSyscall(9, reinterpret_cast<syscall_fn_t>(&User::malloc));
	addSyscall(40, reinterpret_cast<syscall_fn_t>(&User::mallocap));
	addSyscall(42, reinterpret_cast<syscall_fn_t>(&User::physAddr));
	addSyscall(10, reinterpret_cast<syscall_fn_t>(&User::free));

	addSyscall(43, reinterpret_cast<syscall_fn_t>(&User::IPC::getProcessList));
	addSyscall(44, reinterpret_cast<syscall_fn_t>(&User::IPC::waitProcess));

	addSyscall(24, reinterpret_cast<syscall_fn_t>(&User::IPC::processId));
	addSyscall(25, reinterpret_cast<syscall_fn_t>(&User::IPC::processIdByName));
	addSyscall(39, reinterpret_cast<syscall_fn_t>(&User::IPC::processIdByNameBlock));
	addSyscall(12, reinterpret_cast<syscall_fn_t>(&User::IPC::registerName));

	addSyscall(13, reinterpret_cast<syscall_fn_t>(&User::IPC::registerStream));
	addSyscall(14, reinterpret_cast<syscall_fn_t>(&User::IPC::obtainStreamWriter));
	addSyscall(15, reinterpret_cast<syscall_fn_t>(&User::IPC::obtainStreamListener));
	addSyscall(16, reinterpret_cast<syscall_fn_t>(&User::IPC::readStream));
	addSyscall(17, reinterpret_cast<syscall_fn_t>(&User::IPC::readStreamUnblock));
	addSyscall(18, reinterpret_cast<syscall_fn_t>(&User::IPC::writeStream));

	addSyscall(19, reinterpret_cast<syscall_fn_t>(&User::IPC::probeQueue));
	addSyscall(20, reinterpret_cast<syscall_fn_t>(&User::IPC::probeQueueUnblock));
	addSyscall(26, reinterpret_cast<syscall_fn_t>(&User::IPC::probeQueueFor));
	addSyscall(27, reinterpret_cast<syscall_fn_t>(&User::IPC::probeQueueForUnblock));
	addSyscall(21, reinterpret_cast<syscall_fn_t>(&User::IPC::readQueue));
	addSyscall(31, reinterpret_cast<syscall_fn_t>(&User::IPC::grabQueue));
	addSyscall(22, reinterpret_cast<syscall_fn_t>(&User::IPC::shiftQueue));
	addSyscall(23, reinterpret_cast<syscall_fn_t>(&User::IPC::sendQueue));

	addSyscall(35, reinterpret_cast<syscall_fn_t>(&User::Process::fork));
	addSyscall(36, reinterpret_cast<syscall_fn_t>(&User::Process::spawn));
	addSyscall(45, reinterpret_cast<syscall_fn_t>(&User::Process::grantPrivilege));
	addSyscall(46, reinterpret_cast<syscall_fn_t>(&User::Process::grantIOPriv));
	addSyscall(47, reinterpret_cast<syscall_fn_t>(&User::Process::beginExecution));
}

void Syscall::addSyscall(u16 num, syscall_fn_t fn) {
	_syscalls[num] = fn;
}

void Syscall::returnToTask(Tasks::Task *task) {
	_returnTask = task;
}

void Syscall::returnToNextTask() {
	Tasks::Task *nextTask = mu_tasks->prepareFetchNextTask();
	if (nextTask == mu_tasks->current) {
		AkariPanic("TODO: let no 'active' processes being running. i.e. have the ukernel HLT or similar. -- bigger question; why isn't idle task running!? or why is it trying to 'returnToNextTask' from a syscall?");
	}
		
	returnToTask(nextTask);
}

void *Syscall::_handler(struct modeswitch_registers *regs) {
	if (regs->callback.eax >= AKARI_SYSCALL_MAXCALLS)
		AkariPanic("System call greater than maximum requested. TODO: kill requesting process.");
	if (!mu_syscall->_syscalls[regs->callback.eax])
		AkariPanic("Non-existing system call requested.");

	mu_syscall->_returnTask = 0;

	syscall_fn_t call = mu_syscall->_syscalls[regs->callback.eax];
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

	if (mu_syscall->_returnTask) {
		mu_tasks->saveRegisterToTask(mu_tasks->current, regs);
		mu_tasks->current = mu_syscall->_returnTask;
		return mu_tasks->assignInternalTask(mu_tasks->current);
	}

	return regs;
}
