#ifndef __AKARI_HPP__
#define __AKARI_HPP__

#include <AkariConsoleSubsystem.hpp>
#include <AkariMemorySubsystem.hpp>

/**
 * The base class for the kernel services.
 */

class AkariKernel {
	public:
		AkariKernel();

		static void Assert(bool);

		// TODO: linked list of AkariSubsystems.
		AkariConsoleSubsystem *Console;
		AkariMemorySubsystem *Memory;
};

extern AkariKernel *Akari;

#endif

