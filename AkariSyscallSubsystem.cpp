#include <AkariSyscallSubsystem.hpp>
#include <Akari.hpp>
#include <UserCalls.hpp>

AkariSyscallSubsystem::AkariSyscallSubsystem(): _syscalls_assigned(0) {
	Akari->Descriptor->idt->installHandler(0x80, &AkariSyscallSubsystem::_handler);

	addSyscall(0, (void *)&User::putc);
	addSyscall(1, (void *)&User::puts);
	addSyscall(2, (void *)&User::putl);
	addSyscall(3, (void *)&User::getProcessId);
	addSyscall(4, (void *)&User::irqWait);
	addSyscall(5, (void *)&User::irqListen);
	addSyscall(6, (void *)&User::panic);
	addSyscall(7, (void *)&User::registerName);
	addSyscall(8, (void *)&User::registerNode);
}

u8 AkariSyscallSubsystem::versionMajor() const { return 0; }
u8 AkariSyscallSubsystem::versionMinor() const { return 1; }
const char *AkariSyscallSubsystem::versionManufacturer() const { return "Akari"; }
const char *AkariSyscallSubsystem::versionProduct() const { return "Akari Syscall"; }

void AkariSyscallSubsystem::addSyscall(u16 num, void *fn) {
	_syscalls[num] = fn;
}

void AkariSyscallSubsystem::returnToTask(AkariTaskSubsystem::Task *task) {
	_returnTask = task;
}

void AkariSyscallSubsystem::returnToNextTask() {
	returnToTask(Akari->Task->getNextTask());
}

void *AkariSyscallSubsystem::_handler(struct modeswitch_registers *regs) {
	if (regs->callback.eax >= AKARI_SYSCALL_MAXCALLS)
		AkariPanic("System call greater than maximum requested. TODO: kill requesting process.");
	if (!Akari->Syscall->_syscalls[regs->callback.eax])
		AkariPanic("Non-existing system call requested.");

	Akari->Syscall->_returnTask = 0;

	void *call = Akari->Syscall->_syscalls[regs->callback.eax];
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

	if (Akari->Syscall->_returnTask) {
		Akari->Task->saveRegisterToTask(Akari->Task->current, regs);
		Akari->Task->current = Akari->Syscall->_returnTask;
		return Akari->Task->assignInternalTask(Akari->Task->current);
	}

	return regs;
}
