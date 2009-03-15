#ifndef __AKARI_SUBSYSTEM_HPP__
#define __AKARI_SUBSYSTEM_HPP__

#include <arch.hpp>

class Akari;

class AkariSubsystem {
	public:
		AkariSubsystem(Akari *);

		virtual u16 VersionMajor() = 0;
		virtual u16 VersionMinor() = 0;
		// TODO: manufacturer string?
		// TODO: product name?

	private:
		Akari *_kernel;
};

#endif

