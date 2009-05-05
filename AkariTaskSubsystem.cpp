#include <AkariTaskSubsystem.hpp>
#include <Akari.hpp>

AkariTaskSubsystem::AkariTaskSubsystem(): current(0)
{ }

u8 AkariTaskSubsystem::VersionMajor() const { return 0; }
u8 AkariTaskSubsystem::VersionMinor() const { return 1; }
const char *AkariTaskSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariTaskSubsystem::VersionProduct() const { return "Akari Task Manager"; }

void AkariTaskSubsystem::SwitchToUsermode() {
	__asm__ __volatile__("	\
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


AkariTaskSubsystem::Task *AkariTaskSubsystem::Task::BootstrapInitialTask(bool userMode, bool interruptFlag, AkariMemorySubsystem::PageDirectory *pageDirBase) {
	Task *nt = new Task(userMode);
	if (userMode)
		nt->_utks = (u32)Akari->Memory->AllocAligned(USER_TASK_KERNEL_STACK_SIZE) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);

	nt->_pageDir = pageDirBase->Clone();

	return nt;
}

AkariTaskSubsystem::Task *AkariTaskSubsystem::Task::CreateTask(u32 entry, bool userMode, bool interruptFlag, AkariMemorySubsystem::PageDirectory *pageDirBase) {
	Task *nt = new Task(userMode);
	nt->_ks = (u32)Akari->Memory->AllocAligned(USER_TASK_STACK_SIZE) + USER_TASK_STACK_SIZE;

	struct modeswitch_registers *regs = 0;

	if (userMode) {
		nt->_utks = (u32)Akari->Memory->AllocAligned(USER_TASK_KERNEL_STACK_SIZE) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);
		regs = (struct modeswitch_registers *)(nt->_utks);

		regs->useresp = nt->_ks;
		regs->ss = 0x23;		// dsと同じ.. ssがTSSにセットされ、dsは（後で）irq_timer_multitaskに手動によるセットされる
	} else {
		nt->_ks -= sizeof(struct callback_registers);
		regs = (struct modeswitch_registers *)(nt->_ks);
	}

	// only set ->callback.* here, as it may not be a proper modeswitch_registers if this isn't userMode
	regs->callback.ds = userMode ? 0x23 : 0x10;
	regs->callback.edi = regs->callback.esi =
		regs->callback.ebp = regs->callback.esp =
		regs->callback.ebx = regs->callback.edx =
		regs->callback.ecx = regs->callback.eax =
		0;
	regs->callback.eip = entry;
	regs->callback.cs = userMode ? 0x1b : 0x08;
	regs->callback.eflags = interruptFlag ? 0x200 : 0x0;

	nt->_pageDir = pageDirBase->Clone();

	return nt;
}

/*
AkariTaskSubsystem::Task *AkariTaskSubsystem::Task::BootstrapTask(bool existing, bool userMode, bool interruptFlag, AkariMemorySubsystem::PageDirectory *pageDirBase) {
	struct modeswitch_registers regs;
	POSIX::memset(&regs, 0, sizeof(struct modeswitch_registers));

	regs.callback.esp = esp;			// this is not[?] relevant; won't be restored by iret XXX test
	regs.callback.ebp = ebp;
	regs.callback.eip = eip;
	regs.callback.cs = userMode ? 0x1b : 0x8;			// dependent on our own GDT
	regs.callback.eflags = interruptFlag ? 0x200 : 0x0;	// preset eflags

	if (userMode) {
		regs.useresp = esp;
		regs.ss = 0x23;
	}

	Task *nt = new Task(regs, userMode);
	nt->_utks = Akari->Memory->AllocAligned(USER_TASK_KERNEL_STACK_SIZE);	// this MUST be visible from the kernel

	nt->_pageDir = pageDirBase->Clone();

	return nt;
}
*/

AkariTaskSubsystem::Task::Task(bool userMode): next(0), _id(0), _userMode(userMode), _pageDir(0), _ks(0), _utks(0) {
	static u32 lastAssignedId = 0;	// wouldn't be surprised if this needs to be accessible some day
	_id = ++lastAssignedId;
}
