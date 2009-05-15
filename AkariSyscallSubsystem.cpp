#include <AkariSyscallSubsystem.hpp>
#include <Akari.hpp>
#include <UserIO.hpp>
#include <UserTask.hpp>

AkariSyscallSubsystem::AkariSyscallSubsystem(): _syscalls_assigned(0) {
	Akari->Descriptor->_idt->InstallHandler(0x80, &AkariSyscallSubsystem::_handler);

	AddSyscall(0, (void *)&User::IO::Puts);
	AddSyscall(1, (void *)&User::IO::Putl);
	AddSyscall(2, (void *)&User::Task::GetProcessId);
	AddSyscall(3, (void *)&User::Task::IrqWait);
}

u8 AkariSyscallSubsystem::VersionMajor() const { return 0; }
u8 AkariSyscallSubsystem::VersionMinor() const { return 1; }
const char *AkariSyscallSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariSyscallSubsystem::VersionProduct() const { return "Akari Syscall"; }

void AkariSyscallSubsystem::AddSyscall(u16 num, void *fn) {
	_syscalls[num] = fn;
}

void AkariSyscallSubsystem::_handler(struct callback_registers *regs) {
	if (regs->eax >= AKARI_SYSCALL_MAXCALLS)
		AkariPanic("System call greater than maximum requested. TODO: kill requesting process.");
	if (!Akari->Syscall->_syscalls[regs->eax])
		AkariPanic("Non-existing system call requested.");

	void *call = Akari->Syscall->_syscalls[regs->eax];
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
            : "r" (regs->edi), "r" (regs->esi), "r" (regs->edx), "r" (regs->ecx), "r" (regs->ebx), "r" (call)
            : "%ebx");

    regs->eax = ret;
}
