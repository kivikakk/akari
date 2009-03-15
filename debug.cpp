#include <debug.hpp>
#include <arch.hpp>

#define SCREEN_VMEM		0xb8000
#define SCREEN_DEATH	0x4f20

void AkariPanic(const char *message) {
	u16 *i = (u16 *)SCREEN_VMEM;
	for (u16 j = 0; j < 80 * 25; ++j, ++i)
		*i = SCREEN_DEATH;
	
	// "Clever."
	i = (u16 *)SCREEN_VMEM;
	while (*message)
		*((s8 *)i++) = *message++;
	
	while(true)
		asm volatile("hlt");
}

