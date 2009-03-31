#ifndef __ENTRY_HPP__
#define __ENTRY_HPP__

#include <arch.hpp>
#include <multiboot.hpp>

extern "C" {
	extern u8 __kstart, __kend;

	extern multiboot_info_t *AkariMultiboot;

	void AkariEntry();

	struct modeswitch_registers *AkariMicrokernel(struct callback_registers *r);
}

#endif

