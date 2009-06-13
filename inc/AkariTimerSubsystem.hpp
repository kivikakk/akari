#ifndef __AKARI_TIMER_SUBSYSTEM_HPP__
#define __AKARI_TIMER_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>

class AkariTimerSubsystem : public AkariSubsystem {
	public:
		AkariTimerSubsystem();

		u8 versionMajor() const;
		u8 versionMinor() const;
		const char *versionManufacturer() const;
		const char *versionProduct() const;

		void setTimer(u16);
};

#endif

