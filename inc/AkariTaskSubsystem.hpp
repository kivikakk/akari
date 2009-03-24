#ifndef __AKARI_TASK_SUBSYSTEM_HPP__
#define __AKARI_TASK_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>

class AkariTaskSubsystem : public AkariSubsystem {
	public:
		AkariTaskSubsystem();

		u8 VersionMajor() const;
		u8 VersionMinor() const;
		const char *VersionManufacturer() const;
		const char *VersionProduct() const;

		void SwitchToUsermode();
};

#endif

