#ifndef __AKARI_SYSCALL_SUBSYSTEM_HPP__
#define __AKARI_SYSCALL_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>

#define AKARI_SYSCALL_MAXCALLS 256

class AkariSyscallSubsystem : public AkariSubsystem {
	public:
		AkariSyscallSubsystem();

		u8 VersionMajor() const;
		u8 VersionMinor() const;
		const char *VersionManufacturer() const;
		const char *VersionProduct() const;

		void AddSyscall(u16 num, void *fn);

	//protected:
		static void _handler(struct callback_registers *);

		void *_syscalls[AKARI_SYSCALL_MAXCALLS];
		u16 _syscalls_assigned;
};

#endif

