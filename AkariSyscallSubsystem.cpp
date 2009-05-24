#include <AkariSyscallSubsystem.hpp>
#include <Akari.hpp>
#include <UserCalls.hpp>

AkariSyscallSubsystem::AkariSyscallSubsystem(): _syscalls_assigned(0) {
	Akari->Descriptor->_idt->InstallHandler(0x80, &AkariSyscallSubsystem::_handler);

	AddSyscall(0, (void *)&User::Putc);
	AddSyscall(1, (void *)&User::Puts);
	AddSyscall(2, (void *)&User::Putl);
	AddSyscall(3, (void *)&User::GetProcessId);
	AddSyscall(4, (void *)&User::IrqWait);
	AddSyscall(5, (void *)&User::IrqListen);
	AddSyscall(6, (void *)&User::Panic);
}

u8 AkariSyscallSubsystem::VersionMajor() const { return 0; }
u8 AkariSyscallSubsystem::VersionMinor() const { return 1; }
const char *AkariSyscallSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariSyscallSubsystem::VersionProduct() const { return "Akari Syscall"; }

void AkariSyscallSubsystem::AddSyscall(u16 num, void *fn) {
	_syscalls[num] = fn;
}

void AkariSyscallSubsystem::ReturnToTask(AkariTaskSubsystem::Task *task) {
	_returnTask = task;
}

void AkariSyscallSubsystem::ReturnToNextTask() {
	ReturnToTask(Akari->Task->GetNextTask());
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
		Akari->Task->SaveRegisterToTask(Akari->Task->current, regs);
		Akari->Task->current = Akari->Syscall->_returnTask;
		return Akari->Task->AssignInternalTask(Akari->Task->current);
	}

	return regs;
}
