#ifndef __AKARI_HPP__
#define __AKARI_HPP__

#include <AkariMemorySubsystem.hpp>
#include <AkariConsoleSubsystem.hpp>
#include <AkariDescriptorSubsystem.hpp>

/**
 * The base class for the kernel services.
 */

class AkariKernel {
	public:
		AkariKernel();

		static void Assert(bool);

		// TODO: linked list of AkariSubsystems.

		AkariMemorySubsystem *Memory;
		AkariConsoleSubsystem *Console;
		AkariDescriptorSubsystem *Descriptor;
};

extern AkariKernel *Akari;

#endif

