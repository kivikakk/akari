#ifndef __AKARI_SYSCALL_SUBSYSTEM_HPP__
#define __AKARI_SYSCALL_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>
#include <AkariTaskSubsystem.hpp>

#define AKARI_SYSCALL_MAXCALLS 256

class AkariSyscallSubsystem : public AkariSubsystem {
	public:
		AkariSyscallSubsystem();

		u8 versionMajor() const;
		u8 versionMinor() const;
		const char *versionManufacturer() const;
		const char *versionProduct() const;

		void addSyscall(u16 num, void *fn);

		void returnToTask(AkariTaskSubsystem::Task *task);
		void returnToNextTask();

	protected:
		static void *_handler(struct modeswitch_registers *);

		void *_syscalls[AKARI_SYSCALL_MAXCALLS];
		u16 _syscalls_assigned;

		AkariTaskSubsystem::Task *_returnTask;
};

#endif

