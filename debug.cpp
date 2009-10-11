#include <debug.hpp>
#include <arch.hpp>

#define SCREEN_VMEM		0xb8000
#define SCREEN_DEATH	0x4f

void AkariPanic(const char *message) {
	u16 *i = (u16 *)SCREEN_VMEM;
	for (u16 j = 0; j < 80 * 25; ++j, ++i)
		*((u8 *)i + 1) = SCREEN_DEATH;
	
	// "Clever."
	i = (u16 *)SCREEN_VMEM;
	while (*message)
		*((s8 *)i++) = *message++;
	
	AkariHalt();
}

void AkariHalt() {
	asm volatile("cli");
	while (true)
		asm volatile("hlt");
}

