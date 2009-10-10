#ifndef __TIMER_HPP__
#define __TIMER_HPP__

#include <Subsystem.hpp>

class Timer : public Subsystem {
	public:
		Timer();

		u8 versionMajor() const;
		u8 versionMinor() const;
		const char *versionManufacturer() const;
		const char *versionProduct() const;

		void setTimer(u16);
};

#endif

