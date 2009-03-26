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

/**
 * This is largely helper code.
 * The way we end up in user mode (and importantly, the way we end up OUT of user mode [wrt. TSS etc.])
 * will be quite different when we multitask, I imagine.
 *
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
	// note this works with our current stack... hm.
}
 */

AkariTaskSubsystem::Task *AkariTaskSubsystem::Task::BootstrapTask(u32 esp, u32 ebp, u32 eip, AkariMemorySubsystem::PageDirectory *pageDirBase) {
	Task *nt = new Task(esp, ebp, eip);
	nt->_pageDir = pageDirBase->Clone();

	return nt;
}

AkariTaskSubsystem::Task::Task(u32 esp, u32 ebp, u32 eip): _esp(esp), _ebp(ebp), _eip(eip), _id(0), _pageDir(0) {
	static u32 lastAssignedId = 0;	// wouldn't be surprised if this needs to be accessible some day
	_id = ++lastAssignedId;
}
