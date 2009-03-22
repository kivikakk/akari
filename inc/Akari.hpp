#ifndef __AKARI_HPP__
#define __AKARI_HPP__

#include <AkariMemorySubsystem.hpp>
#include <AkariConsoleSubsystem.hpp>
#include <AkariDescriptorSubsystem.hpp>
#include <AkariTimerSubsystem.hpp>

/**
 * The base class for the kernel services.
 */

class AkariKernel {
	public:
		static AkariKernel *Construct(u32, u32);

		// TODO: linked list of AkariSubsystems so we can iterate them generically.

		AkariMemorySubsystem *Memory;
		AkariConsoleSubsystem *Console;
		AkariDescriptorSubsystem *Descriptor;
		AkariTimerSubsystem *Timer;
	
	protected:
		AkariKernel();
};

extern AkariKernel *Akari;

#endif

