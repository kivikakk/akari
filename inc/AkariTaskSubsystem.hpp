#ifndef __AKARI_TASK_SUBSYSTEM_HPP__
#define __AKARI_TASK_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>
#include <AkariMemorySubsystem.hpp>
#include <interrupts.hpp>

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
				static Task *BootstrapTask(u32, u32, u32, u8, bool, AkariMemorySubsystem::PageDirectory *);

				Task *next;

			// protected:
				Task(const struct modeswitch_registers &, bool);

				struct modeswitch_registers _registers;
				// only use `callback' part of it if we're not usermode.

				u32 _id;
				bool _userMode;

				AkariMemorySubsystem::PageDirectory *_pageDir;
		};

		Task *start, *current;
};

#endif

