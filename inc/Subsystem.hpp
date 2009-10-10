#ifndef __SUBSYSTEM_HPP__
#define __SUBSYSTEM_HPP__

#include <arch.hpp>

class Subsystem {
	public:
		Subsystem();
		virtual ~Subsystem();

		virtual u8 versionMajor() const = 0;
		virtual u8 versionMinor() const = 0;
		virtual const char *versionManufacturer() const = 0;
		virtual const char *versionProduct() const = 0;
};

#endif

