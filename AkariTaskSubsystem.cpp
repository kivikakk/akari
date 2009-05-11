#include <AkariTaskSubsystem.hpp>
#include <Akari.hpp>

AkariTaskSubsystem::AkariTaskSubsystem(): current(0)
{ }

u8 AkariTaskSubsystem::VersionMajor() const { return 0; }
u8 AkariTaskSubsystem::VersionMinor() const { return 1; }
const char *AkariTaskSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariTaskSubsystem::VersionProduct() const { return "Akari Task Manager"; }

void AkariTaskSubsystem::SwitchToUsermode(u8 iopl) {
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
		"
		// is this bit below even necessary?
		" \
		xor %%bh, %%bh; \
		shl $12, %%bx; \
		or %%bx, %%ax; \
		\
		push %%eax; \
		pushl $0x1b; \
		pushl $1f; \
		\
		iret; \
	1:" : : "b" (iopl) : "%eax");

	// EIP ($1f), CS (1b), EFLAGS (eax), ESP (eax from before), SS (23)
	// note this works with our current stack... hm.
}


AkariTaskSubsystem::Task *AkariTaskSubsystem::Task::BootstrapInitialTask(bool userMode, AkariMemorySubsystem::PageDirectory *pageDirBase) {
	Task *nt = new Task(userMode);
	if (userMode)
		nt->_utks = (u32)Akari->Memory->AllocAligned(USER_TASK_KERNEL_STACK_SIZE) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);
	else {
		AkariPanic("I haven't tested a non-user idle (initial) task. Uncomment this panic at your own peril.");
		// i.e. you may need to add some code as deemed appropriate here. Current thoughts are that you may need to
		// be careful about where you placed the stack.. probably not, but just check it all matches up?
	}

	nt->_pageDir = pageDirBase->Clone();

	return nt;
}

AkariTaskSubsystem::Task *AkariTaskSubsystem::Task::CreateTask(u32 entry, bool userMode, bool interruptFlag, u8 iopl, AkariMemorySubsystem::PageDirectory *pageDirBase) {
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
	regs->callback.cs = userMode ? 0x1b : 0x08;			// note the low 2 bits are the CPL
	regs->callback.eflags = (interruptFlag ? 0x200 : 0x0) | (iopl << 12);

	nt->_pageDir = pageDirBase->Clone();

	return nt;
}

bool AkariTaskSubsystem::Task::GetIOMap(u8 port) const {
	return (_iomap[port / 8] & (1 << (port % 8))) == 0;
}

void AkariTaskSubsystem::Task::SetIOMap(u8 port, bool enabled) {
	if (enabled)
		_iomap[port / 8] &= ~(1 << (port % 8));
	else
		_iomap[port / 8] |= (1 << (port % 8));
}

AkariTaskSubsystem::Task::Task(bool userMode): next(0), _id(0), _userMode(userMode), _pageDir(0), _ks(0), _utks(0) {
	static u32 lastAssignedId = 0;	// wouldn't be surprised if this needs to be accessible some day
	_id = ++lastAssignedId;

	for (u8 i = 0; i < 32; ++i)
		_iomap[i] = 0xFF;
}
