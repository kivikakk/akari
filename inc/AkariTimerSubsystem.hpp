#ifndef __AKARI_TIMER_SUBSYSTEM_HPP__
#define __AKARI_TIMER_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>

class AkariTimerSubsystem : public AkariSubsystem {
	public:
		AkariTimerSubsystem();

		u8 VersionMajor() const;
		u8 VersionMinor() const;
		const char *VersionManufacturer() const;
		const char *VersionProduct() const;

		void SetTimer(u16);
};

#endif

