#ifndef __AKARI_TASK_SUBSYSTEM_HPP__
#define __AKARI_TASK_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>
#include <AkariMemorySubsystem.hpp>
#include <interrupts.hpp>

#define USER_TASK_KERNEL_STACK_SIZE	0x1000

class AkariTaskSubsystem : public AkariSubsystem {
	public:
		AkariTaskSubsystem();

		u8 VersionMajor() const;
		u8 VersionMinor() const;
		const char *VersionManufacturer() const;
		const char *VersionProduct() const;

		static void SwitchToUsermode();

		class Task {
			public:
				static Task *BootstrapTask(u32 esp, u32 ebp, u32 eip, bool userMode, bool interruptFlag, AkariMemorySubsystem::PageDirectory *pageDirBase);

				Task *next;

			// protected:
				Task(const struct modeswitch_registers &registers, bool userMode);

				struct modeswitch_registers _registers;
				// only use `callback' part of it if we're not usermode.

				u32 _id;
				bool _userMode;

				AkariMemorySubsystem::PageDirectory *_pageDir;
				u32 _utks;
		};

		Task *start, *current;
};

#endif

