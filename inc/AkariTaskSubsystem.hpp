#ifndef __AKARI_TASK_SUBSYSTEM_HPP__
#define __AKARI_TASK_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>
#include <AkariMemorySubsystem.hpp>
#include <interrupts.hpp>

// user task kernel stack is used for state when it's
// pre-empted, and for system calls, etc.
#define USER_TASK_KERNEL_STACK_SIZE	0x2000
#define USER_TASK_STACK_SIZE	0x4000

class AkariTaskSubsystem : public AkariSubsystem {
	public:
		AkariTaskSubsystem();

		u8 VersionMajor() const;
		u8 VersionMinor() const;
		const char *VersionManufacturer() const;
		const char *VersionProduct() const;

		static void SwitchRing(u8 cpl, u8 iopl);

		void *CycleTask(void *r);

		class Task {
			public:
				static Task *BootstrapInitialTask(u8 cpl, AkariMemorySubsystem::PageDirectory *pageDirBase);
				static Task *CreateTask(u32 entry, u8 cpl, bool interruptFlag, u8 iopl, AkariMemorySubsystem::PageDirectory *pageDirBase);

				bool GetIOMap(u8 port) const;
				void SetIOMap(u8 port, bool enabled);

				Task *next, *priorityNext;

				u32 irqWait;

			// protected:
				Task(u8 cpl);

				u32 _id; u8 _cpl;

				AkariMemorySubsystem::PageDirectory *_pageDir;

				u32 _ks; u32 _utks;
				u8 _iomap[32];
		};

		Task *start, *current;
		Task *priorityStart;
};

#endif

