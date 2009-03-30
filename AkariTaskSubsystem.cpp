#include <AkariTaskSubsystem.hpp>
#include <Akari.hpp>

AkariTaskSubsystem::AkariTaskSubsystem(): current(0)
{ }

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

AkariTaskSubsystem::Task *AkariTaskSubsystem::Task::BootstrapTask(u32 esp, u32 ebp, u32 eip, u8 cs, bool interruptFlag, AkariMemorySubsystem::PageDirectory *pageDirBase) {
	struct callback_registers regs;
	POSIX::memset(&regs, 0, sizeof(struct callback_registers));

	regs.esp = esp;			// this is not relevant; won't be restored by iret
	regs.ebp = ebp;
	regs.eip = eip;
	regs.cs = cs;
	regs.eflags = interruptFlag ? 0x200 : 0x0;	// preset eflags

	// vv   is this just plain wrong? probably. think about this more next time
	// regs.useresp = esp;		// XXX: this is more important? or only for less privileged tasks?

	Task *nt = new Task(regs);
	nt->_pageDir = pageDirBase->Clone();

	return nt;
}

AkariTaskSubsystem::Task::Task(const struct callback_registers &registers): next(0), _registers(registers), _id(0), _pageDir(0) {
	static u32 lastAssignedId = 0;	// wouldn't be surprised if this needs to be accessible some day
	_id = ++lastAssignedId;
}
