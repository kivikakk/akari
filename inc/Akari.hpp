#ifndef __AKARI_HPP__
#define __AKARI_HPP__

#include <arch.hpp>

class Memory;
class Console;
class Descriptor;
class Timer;
class Tasks;
class Syscall;

/**
 * The base class for the kernel services.
 */
class Kernel {
	public:
		static Kernel *Construct(u32, u32);

		// TODO: linked list of Subsystems so we can iterate them generically.

		Memory *memory;
		Console *console;
		Descriptor *descriptor;
		Timer *timer;
		Tasks *tasks;
		Syscall *syscall;
	
	protected:
		Kernel();
};

extern Kernel *Akari;

#endif

