#ifndef __AKARI_HPP__
#define __AKARI_HPP__

#include <AkariMemorySubsystem.hpp>

/**
 * The base class for the kernel services.
 */

class Akari {
	public:
		Akari();

		static void Assert(bool);

		// TODO: linked list of AkariSubsystems.
		AkariConsoleSubsystem *Console;
		AkariMemorySubsystem *Memory;
};

extern Akari *Kernel;

#endif

