#include <Tasks.hpp>
#include <Akari.hpp>
#include <Descriptor.hpp>

Tasks::Tasks(): start(0), current(0), priorityStart(0) {
	registeredTasks = new HashTable<Symbol, Task *>();
}

u8 Tasks::versionMajor() const { return 0; }
u8 Tasks::versionMinor() const { return 1; }
const char *Tasks::versionManufacturer() const { return "Akari"; }
const char *Tasks::versionProduct() const { return "Akari Task Manager"; }

void Tasks::SwitchRing(u8 cpl, u8 iopl) {
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

static inline Tasks::Task *_NextTask(Tasks::Task *t) {
	return t->next ? t->next : Akari->tasks->start;
}

Tasks::Task *Tasks::getNextTask() {
	if (priorityStart) {
		// We have something priority. We put it in without regard for irqWait,
		// since it's probably an IRQ being fired that put it there ...

		Task *t = priorityStart;
		priorityStart = t->priorityNext;
		t->priorityNext = 0;
		return t;
	} else {
		Task *t = _NextTask(current);

		// if the new current task is irqWait, loop until we find one which isn't.
		// be sure not to be caught in an infinite loop by seeing if we get back to
		// the same one again. (i.e. every task is irqWait)
		// it intentionally will be able to loop back to the task we switched from. (though
		// of course, it won't if it's irqWait ...)

		if (t->irqWaiting && !t->irqListenHits) {
			Task *newCurrent = t;
			while (newCurrent->irqWaiting && !t->irqListenHits) {
				Task *next = _NextTask(newCurrent);
				ASSERT(next != t);
				newCurrent = next;
			}
			t = newCurrent;
		}

		return t;
	}
}

void Tasks::cycleTask() {
	current = getNextTask();
}

void Tasks::saveRegisterToTask(Task *dest, void *regs) {
	// we're saving a task even though it's in a state not matching its CPL?
	// This could happen if, say, it's in kernel mode, then interrupts or something.
	// HACK: need to handle this situation properly, but for now, ensure
	// internal consistency.
	ASSERT(((((struct modeswitch_registers *)regs)->callback.cs - 0x08) / 0x11) == dest->cpl);

	if (!dest->cpl > 0) {
		// update the tip of stack pointer so we can restore later
		dest->ks = (u32)regs;
	} else {
		// update utks pointer
		dest->utks = (u32)regs;
	}
}

void *Tasks::assignInternalTask(Task *task) {
	// now set the page directory, utks for TSS (if applicable) and stack to switch to as appropriate
	
	// TODO: let task be NUL and just HLT until an interrupt/timer/...?
	ASSERT(task);
	Akari->memory->switchPageDirectory(task->pageDir);
	if (task->cpl > 0) {
		Akari->descriptor->gdt->setTSSStack(task->utks + sizeof(struct modeswitch_registers));
		Akari->descriptor->gdt->setTSSIOMap(task->iomap);
	}

	if (task->irqWaiting) {
		task->irqWaiting = false;
		task->irqListenHits--;
	}

	return (void *)((!task->cpl > 0) ? task->ks : task->utks);
}

Tasks::Task *Tasks::Task::BootstrapInitialTask(u8 cpl, Memory::PageDirectory *pageDirBase) {
	Task *nt = new Task(cpl);
	if (cpl > 0)
		nt->utks = (u32)Akari->memory->allocAligned(USER_TASK_KERNEL_STACK_SIZE) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);
	else {
		// nt->ks = (u32)Akari->memory->allocAligned(USER_TASK_KERNEL_STACK_SIZE) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);
		AkariPanic("I haven't tested a non-user initial task. Uncomment this panic at your own peril.");
		// i.e. you may need to add some code as deemed appropriate here. Current thoughts are that you may need to
		// be careful about where you placed the stack.. probably not, but just check it all matches up?
		// ALSO NOTE WELL: SwitchRing does a range of important things like enabling interrupts,
		// which is good for, you know, multitasking. So if you're going to do this, be careful ...
	}

	nt->pageDir = pageDirBase->clone();

	return nt;
}

Tasks::Task *Tasks::Task::CreateTask(u32 entry, u8 cpl, bool interruptFlag, u8 iopl, Memory::PageDirectory *pageDirBase) {
	Task *nt = new Task(cpl);
	nt->ks = (u32)Akari->memory->allocAligned(USER_TASK_STACK_SIZE) + USER_TASK_STACK_SIZE;

	struct modeswitch_registers *regs = 0;

	if (cpl > 0) {
		nt->utks = (u32)Akari->memory->allocAligned(USER_TASK_KERNEL_STACK_SIZE) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);
		regs = (struct modeswitch_registers *)(nt->utks);

		regs->useresp = nt->ks;
		regs->ss = 0x10 + (cpl * 0x11);		// dsと同じ.. ssがTSSにセットされ、dsは（後で）irq_timer_multitaskに手動によるセットされる
	} else {
		nt->ks -= sizeof(struct callback_registers);
		regs = (struct modeswitch_registers *)(nt->ks);
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

	nt->pageDir = pageDirBase->clone();

	return nt;
}

bool Tasks::Task::getIOMap(u8 port) const {
	return (iomap[port / 8] & (1 << (port % 8))) == 0;
}

void Tasks::Task::setIOMap(u8 port, bool enabled) {
	if (enabled)
		iomap[port / 8] &= ~(1 << (port % 8));
	else
		iomap[port / 8] |= (1 << (port % 8));
}

Tasks::Task::Node::Node(): _wl_id(0) {
}

u32 Tasks::Task::Node::registerWriter(bool exclusive) {
	if (_exclusive) return -1;

	if (exclusive) {
		if ((_writers.begin() != _writers.end())) {
			// Whoops; there are already writers! Can't set it exclusive now.
			return -1;
		}
		_exclusive = true;
	}

	u32 id = _nextId();
	_writers.push_back(id);
	return id;
}


u32 Tasks::Task::Node::registerListener() {
	u32 id = _nextId();
	_writers.push_back(id);
	return id;
}

bool Tasks::Task::Node::hasWriter(u32 id) const {
	for (const LinkedList<u32>::iterator it = _writers.begin(); it != _writers.end(); ++it) {
		if (*it == id)
			return true;
	}
	return false;
}

bool Tasks::Task::Node::hasListener(u32 id) const {
	for (const LinkedList<u32>::iterator it = _listeners.begin(); it != _listeners.end(); ++it) {
		if (*it == id)
			return true;
	}
	return false;
}

u32 Tasks::Task::Node::_nextId() {
	return ++_wl_id;
}

Tasks::Task::Task(u8 cpl):
		next(0), priorityNext(0), irqWaiting(false), irqListen(0), irqListenHits(0),
		id(0), registeredName(), cpl(cpl), pageDir(0), ks(0), utks(0) {
	static u32 lastAssignedId = 0;	// wouldn't be surprised if this needs to be accessible some day
	id = ++lastAssignedId;

	for (u8 i = 0; i < 32; ++i)
		iomap[i] = 0xFF;

	nodesByName = new HashTable<Symbol, Node *>();
}
