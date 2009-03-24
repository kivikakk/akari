#include <AkariTaskSubsystem.hpp>
#include <Akari.hpp>

AkariTaskSubsystem::AkariTaskSubsystem() {
	u32 a;
	Akari->Console->PutString("stack variable is at 0x");
	Akari->Console->PutInt((u32)&a, 16);
	Akari->Console->PutChar('\n');
	Akari->Console->PutString("Akari is at 0x");
	Akari->Console->PutInt((u32)Akari, 16);
	Akari->Console->PutChar('\n');
	Akari->Console->PutString("&Akari is at 0x");
	Akari->Console->PutInt((u32)&Akari, 16);
	Akari->Console->PutChar('\n');
}

u8 AkariTaskSubsystem::VersionMajor() const { return 0; }
u8 AkariTaskSubsystem::VersionMinor() const { return 1; }
const char *AkariTaskSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariTaskSubsystem::VersionProduct() const { return "Akari Task Manager"; }

void AkariTaskSubsystem::SwitchToUsermode() {
	__asm__ __volatile__("	\
		cli; \
		mov $0x23, %%ax; \
		mov %%ax, %%ds; \
		mov %%ax, %%es; \
		mov %%ax, %%fs; \
		mov %%ax, %%gs; \
\
		mov %%esp, %%eax; \
		pushl $0x23; \
		pushl %%eax; \
		pushf; \
		pop %%eax; \
		or $0x200, %%eax; \
		push %%eax; \
		pushl $0x1b; \
		pushl $1f; \
\
		iret; \
	1:" : : : "%eax");

	// EIP ($1f), CS (1b), EFLAGS (eax), ESP (eax from before), SS (23)
}

