#include <AkariSyscallSubsystem.hpp>
#include <Akari.hpp>
#include <UserIO.hpp>

AkariSyscallSubsystem::AkariSyscallSubsystem(): _syscalls_assigned(0) {
	Akari->Descriptor->_idt->InstallHandler(0x80, &AkariSyscallSubsystem::_handler);

	AddSyscall(0, (void *)&User::IO::Puts);
}

u8 AkariSyscallSubsystem::VersionMajor() const { return 0; }
u8 AkariSyscallSubsystem::VersionMinor() const { return 1; }
const char *AkariSyscallSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariSyscallSubsystem::VersionProduct() const { return "Akari Syscall"; }

void AkariSyscallSubsystem::AddSyscall(u16 num, void *fn) {
	_syscalls[num] = fn;
}

void AkariSyscallSubsystem::_handler(struct callback_registers regs) {
	if (regs.eax >= AKARI_SYSCALL_MAXCALLS || !Akari->Syscall->_syscalls[regs.eax]) {
		return;
	}

	void *call = Akari->Syscall->_syscalls[regs.eax];
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
            : "r" (regs.edi), "r" (regs.esi), "r" (regs.edx), "r" (regs.ecx), "r" (regs.ebx), "r" (call)
            : "%ebx");

    regs.eax = ret; // ??
}
