#include <AkariTaskSubsystem.hpp>
#include <Akari.hpp>

AkariTaskSubsystem::AkariTaskSubsystem(): start(0), current(0), priorityStart(0)
{ }

u8 AkariTaskSubsystem::VersionMajor() const { return 0; }
u8 AkariTaskSubsystem::VersionMinor() const { return 1; }
const char *AkariTaskSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariTaskSubsystem::VersionProduct() const { return "Akari Task Manager"; }

void AkariTaskSubsystem::SwitchRing(u8 cpl, u8 iopl) {
	// This code works by fashioning the stack to be right for a cross-privilege IRET into
	// the ring, IOPL, etc. of our choice, simultaneously enabling interrupts.

	// AX has the offset in the GDT to the appropriate **data** segment.
	// It's copied to DS, ES, FS, GS.
	// We record the current ESP in EBX (clobbered). We push the DS (will become SS [? check]).
	// We then push the recorded ESP, then pushf.
	// We pop the last item from pushf (EFLAGS) into EBX (since we clobbered it anyway), and
	// enable interrupts by setting a bit in it.

	// CX contains the IOPL. It needs to start 12 bits left in EFLAGS, so we shift it in place
	// and then OR it into BX. EBX now contains an appropriately twiddled EFLAGS.
	
	// EBX is pushed back onto the stack to make that pushf complete (and slightly molested).
	// We rachet EAX back 8 bytes so it points at the right **code** segment (CS), push that,
	// and then lastly push our EIP (which is conveniently the next instruction).
	// Then we IRET and we wind up with everything great.

	// Note that the __asm__ __volatile__ is likely to try to restore a couple registers.
	// This seems kinda dangerous, and I'd look to rewriting the entire call in assembly
	// just so the compiler doesn't try to do anything tricky on me.

	__asm__ __volatile__("	\
		mov %%ax, %%ds; \
		mov %%ax, %%es; \
		mov %%ax, %%fs; \
		mov %%ax, %%gs; \
		\
		mov %%esp, %%ebx; \
		pushl %%eax; \
		pushl %%ebx; \
		pushf; \
		pop %%ebx; \
		or $0x200, %%ebx; \
		\
		shl $12, %%cx; \
		or %%cx, %%bx; \
		\
		push %%ebx; \
		sub $0x8, %%eax; \
		pushl %%eax; \
		pushl $1f; \
		\
		iret; \
	1:" : : "a" (0x10 + (0x11 * cpl)), "c" (iopl) : "%ebx");

	// note this works with our current stack... hm.
}

static inline AkariTaskSubsystem::Task *_NextTask(AkariTaskSubsystem::Task *t) {
	return t->next ? t->next : Akari->Task->start;
}

void *AkariTaskSubsystem::CycleTask(void *r) {
	if (!current->_cpl > 0) {
		// update the tip of stack pointer so we can restore later
		current->_ks = (u32)r;
	} else {
		// update utks pointer
		current->_utks = (u32)r;
	}
	
	if (priorityStart) {
		// We have something priority. We put it in without regard for irqWait,
		// since it's probably an IRQ being fired that put it there ...

		current = priorityStart;
		priorityStart = current->priorityNext;
		current->priorityNext = 0;
	} else {
		current = _NextTask(current);

		// if the new current task is irqWait, loop until we find one which isn't.
		// be sure not to be caught in an infinite loop by seeing if we get back to
		// the same one again. (i.e. every task is irqWait)
		// it intentionally will be able to loop back to the task we switched from. (though
		// of course, it won't if it's irqWait ...)

		if (current->irqWait) {
			Task *newCurrent = current;
			while (newCurrent->irqWait) {
				Task *next = _NextTask(newCurrent);
				ASSERT(next != current);
				newCurrent = next;
			}
			current = newCurrent;
		}
	}

	// now set the page directory, utks for TSS (if applicable) and stack to switch to as appropriate
	Akari->Memory->SwitchPageDirectory(current->_pageDir);
	if (current->_cpl > 0) {
		Akari->Descriptor->_gdt->SetTSSStack(current->_utks + sizeof(struct modeswitch_registers));
		Akari->Descriptor->_gdt->SetTSSIOMap(current->_iomap);
	}

	return (void *)((!current->_cpl > 0) ? current->_ks : current->_utks);
}


AkariTaskSubsystem::Task *AkariTaskSubsystem::Task::BootstrapInitialTask(u8 cpl, AkariMemorySubsystem::PageDirectory *pageDirBase) {
	Task *nt = new Task(cpl);
	if (cpl > 0)
		nt->_utks = (u32)Akari->Memory->AllocAligned(USER_TASK_KERNEL_STACK_SIZE) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);
	else {
		AkariPanic("I haven't tested a non-user idle (initial) task. Uncomment this panic at your own peril.");
		// i.e. you may need to add some code as deemed appropriate here. Current thoughts are that you may need to
		// be careful about where you placed the stack.. probably not, but just check it all matches up?
	}

	nt->_pageDir = pageDirBase->Clone();

	return nt;
}

AkariTaskSubsystem::Task *AkariTaskSubsystem::Task::CreateTask(u32 entry, u8 cpl, bool interruptFlag, u8 iopl, AkariMemorySubsystem::PageDirectory *pageDirBase) {
	Task *nt = new Task(cpl);
	nt->_ks = (u32)Akari->Memory->AllocAligned(USER_TASK_STACK_SIZE) + USER_TASK_STACK_SIZE;

	struct modeswitch_registers *regs = 0;

	if (cpl > 0) {
		nt->_utks = (u32)Akari->Memory->AllocAligned(USER_TASK_KERNEL_STACK_SIZE) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);
		regs = (struct modeswitch_registers *)(nt->_utks);

		regs->useresp = nt->_ks;
		regs->ss = 0x10 + (cpl * 0x11);		// dsと同じ.. ssがTSSにセットされ、dsは（後で）irq_timer_multitaskに手動によるセットされる
	} else {
		nt->_ks -= sizeof(struct callback_registers);
		regs = (struct modeswitch_registers *)(nt->_ks);
	}

	// only set ->callback.* here, as it may not be a proper modeswitch_registers if cpl==0
	regs->callback.ds = 0x10 + (cpl * 0x11);
	regs->callback.edi = regs->callback.esi =
		regs->callback.ebp = regs->callback.esp =
		regs->callback.ebx = regs->callback.edx =
		regs->callback.ecx = regs->callback.eax =
		0;
	regs->callback.eip = entry;
	regs->callback.cs = 0x08 + (cpl * 0x11);			// note the low 2 bits are the CPL
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

AkariTaskSubsystem::Task::Task(u8 cpl): next(0), priorityNext(0), irqWait(0), _id(0), _cpl(cpl), _pageDir(0), _ks(0), _utks(0) {
	static u32 lastAssignedId = 0;	// wouldn't be surprised if this needs to be accessible some day
	_id = ++lastAssignedId;

	for (u8 i = 0; i < 32; ++i)
		_iomap[i] = 0xFF;
}
