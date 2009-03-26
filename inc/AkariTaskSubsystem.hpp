#ifndef __AKARI_TASK_SUBSYSTEM_HPP__
#define __AKARI_TASK_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>
#include <AkariMemorySubsystem.hpp>

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

			protected:
				Task(u32, u32, u32);

				u32 _esp, _ebp, _eip;
				u32 _id;

				AkariMemorySubsystem::PageDirectory *_pageDir;
		};
};

#endif

