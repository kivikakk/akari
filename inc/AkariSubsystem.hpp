#ifndef __AKARI_SUBSYSTEM_HPP__
#define __AKARI_SUBSYSTEM_HPP__

#include <arch.hpp>

class Akari;

class AkariSubsystem {
	public:
		AkariSubsystem(Akari *);

		virtual u16 VersionMajor() const = 0;
		virtual u16 VersionMinor() const = 0;
		// TODO: manufacturer string?
		// TODO: product name?

		Akari *_kernel;
};

#endif

