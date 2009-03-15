#ifndef __AKARI_SUBSYSTEM_HPP__
#define __AKARI_SUBSYSTEM_HPP__

#include <arch.hpp>

class AkariSubsystem {
	public:
		AkariSubsystem();

		virtual u8 VersionMajor() const = 0;
		virtual u8 VersionMinor() const = 0;
		virtual const char *VersionManufacturer() const = 0;
		virtual const char *VersionProduct() const = 0;
};

#endif

