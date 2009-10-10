#include <Syscall.hpp>
#include <Akari.hpp>
#include <UserCalls.hpp>
#include <Descriptor.hpp>

Syscall::Syscall(): _syscalls_assigned(0) {
	Akari->descriptor->idt->installHandler(0x80, &Syscall::_handler);

	addSyscall(0, (void *)&User::putc);
	addSyscall(1, (void *)&User::puts);
	addSyscall(2, (void *)&User::putl);
	addSyscall(3, (void *)&User::getProcessId);
	addSyscall(4, (void *)&User::irqWait);
	addSyscall(5, (void *)&User::irqListen);
	addSyscall(6, (void *)&User::panic);
	addSyscall(7, (void *)&User::registerName);
	addSyscall(8, (void *)&User::registerNode);
	addSyscall(9, (void *)&User::exit);
	addSyscall(10, (void *)&User::obtainNodeWriter);
	addSyscall(11, (void *)&User::obtainNodeListener);
	addSyscall(12, (void *)&User::readNode);
	addSyscall(13, (void *)&User::writeNode);
	addSyscall(14, (void *)&User::defer);
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
	Tasks::Task *nextTask = Akari->tasks->getNextTask();
	if (nextTask == Akari->tasks->current) {
		AkariPanic("TODO: let no 'active' processes being running. i.e. have the ukernel HLT or similar.");
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
