#include <debug.hpp>
#include <arch.hpp>

#define SCREEN_VMEM		0xb8000
#define SCREEN_DEATH	0x4f

static const char *PANIC_MSG = "oh god how did this get here I am not good with kernel panic";

void AkariPanic(const char *message) {
	u16 *i = (u16 *)SCREEN_VMEM;
	for (u16 j = 0; j < 80 * 25; ++j, ++i)
		*((u8 *)i + 1) = SCREEN_DEATH;
	
	// "Clever."
	i = (u16 *)SCREEN_VMEM;
	while (*PANIC_MSG)
		*((s8 *)i++) = *PANIC_MSG++;

	i = (u16 *)SCREEN_VMEM + 80;
	while (*message)
		*((s8 *)i++) = *message++;
	
	AkariHalt();
}

void AkariHalt() {
	asm volatile("cli");
	while (true)
		asm volatile("hlt");
}

