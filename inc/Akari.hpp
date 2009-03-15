#ifndef __AKARI_HPP__
#define __AKARI_HPP__

#include <AkariMemorySubsystem.hpp>

/**
 * The base class for the kernel services.
 */

class Akari {
	public:
		Akari();

		// TODO: list of AkariSubsystems.
		AkariMemorySubsystem *Memory;
};

extern Akari *Kernel;

#endif

