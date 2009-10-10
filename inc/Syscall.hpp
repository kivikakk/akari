#ifndef __SYSCALL_HPP__
#define __SYSCALL_HPP__

#include <Subsystem.hpp>
#include <Tasks.hpp>

#define AKARI_SYSCALL_MAXCALLS 256

class Syscall : public Subsystem {
	public:
		Syscall();

		u8 versionMajor() const;
		u8 versionMinor() const;
		const char *versionManufacturer() const;
		const char *versionProduct() const;

		void addSyscall(u16 num, void *fn);

		void returnToTask(Tasks::Task *task);
		void returnToNextTask();

	protected:
		static void *_handler(struct modeswitch_registers *);

		void *_syscalls[AKARI_SYSCALL_MAXCALLS];
		u16 _syscalls_assigned;

		Tasks::Task *_returnTask;
};

#endif

