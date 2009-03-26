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

		// void SwitchToUsermode();		XXX later

		class Task {
			public:
				static Task *BootstrapTask(u32, u32, u32, AkariMemorySubsystem::PageDirectory *);

				Task *next;

			// protected:
				Task(const struct registers &);

				struct registers _registers;
				u32 _id;

				AkariMemorySubsystem::PageDirectory *_pageDir;
		};

		Task *start, *current;
};

#endif

