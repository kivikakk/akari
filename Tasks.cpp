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

		if ((t->irqWaiting && !t->irqListenHits) || t->userWaiting) {
			Task *newCurrent = t;
			while ((newCurrent->irqWaiting && !newCurrent->irqListenHits) || newCurrent->userWaiting) {
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

	// update the tip of stack pointer so we can restore later
	// Akari->console->putString("ks saved as 0x");
	// Akari->console->putInt((u32)regs, 16);
	// Akari->console->putString("\n");
	dest->ks = (u32)regs;
}

void *Tasks::assignInternalTask(Task *task) {
	// now set the page directory, ks for TSS (if applicable) and stack to switch to as appropriate
	
	ASSERT(task);
	Akari->memory->switchPageDirectory(task->pageDir);

	if (!task->heap && task->heapStart != task->heapMax) {
		task->heap = new Memory::Heap(task->heapStart, task->heapEnd, task->heapMax, false, false);	// false false? XXX Always?
	}

	if (task->cpl > 0) {
		Akari->descriptor->gdt->setTSSStack(task->ks + sizeof(struct modeswitch_registers));
		Akari->descriptor->gdt->setTSSIOMap(task->iomap);
	}

	if (task->irqWaiting) {
		task->irqWaiting = false;
		task->irqListenHits--;
	}

	if (task->userWaiting) {
		AkariPanic("task->userWaiting");			// We shouldn't be switching to a task that's waiting. Block fail?
	} else if (task->userCall) {
		// We want to change the value in the stack which actually becomes the return value of the syscall.
		// If they're a kernel proc (cpl==0), then that's just the EAX on the ks.
		struct modeswitch_registers *regs = (struct modeswitch_registers *)task->ks;
		regs->callback.eax = (*task->userCall)();
		ASSERT(!task->userCall->shallBlock());

		delete task->userCall;
		task->userCall = 0;
	}

	return (void *)task->ks;
}

Tasks::Task *Tasks::Task::BootstrapInitialTask(u8 cpl, Memory::PageDirectory *pageDirBase) {
	Task *nt = new Task(cpl);
	if (cpl > 0)
		nt->ks = (u32)Akari->memory->allocAligned(USER_TASK_KERNEL_STACK_SIZE) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);
	else {
		// nt->ks = (u32)Akari->memory->allocAligned(USER_TASK_KERNEL_STACK_SIZE) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);
		AkariPanic("I haven't tested a non-user initial task. Uncomment this panic at your own peril.");
		// i.e. you may need to add some code as deemed appropriate here. Current thoughts are that you may need to
		// be careful about where you placed the stack.. probably not, but just check it all matches up?
		// ALSO NOTE WELL: SwitchRing does a range of important things like enabling interrupts,
		// which is good for, you know, multitasking. So if you're going to do this, be careful ...
		// ALSO NEXT: It doesn't really matter since we're killing the init task pretty quickly anyway..
	}

	nt->pageDir = pageDirBase->clone();
	// We don't give it a heap, it won't need one ...

	return nt;
}

Tasks::Task *Tasks::Task::CreateTask(u32 entry, u8 cpl, bool interruptFlag, u8 iopl, Memory::PageDirectory *pageDirBase) {
	Task *nt = new Task(cpl);

	nt->pageDir = pageDirBase->clone();

	struct modeswitch_registers *regs = 0;

	if (cpl > 0) {
		// Allocate the user's stack to top out at USER_TASK_BASE (i.e. it goes below that).
		// We need to do the frame allocs ourselves so they're visible and writable by the user.
		for (u32 i = 0, page = USER_TASK_BASE - 0x1000; i < USER_TASK_STACK_SIZE; i += 0x1000, page -= 0x1000) {
			nt->pageDir->getPage(page, true)->allocAnyFrame(false, true);
		}

		nt->ks = (u32)Akari->memory->allocAligned(USER_TASK_KERNEL_STACK_SIZE) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);
		regs = (struct modeswitch_registers *)(nt->ks);

		Akari->console->putString("alloced nt->ks at 0x");
		Akari->console->putInt(nt->ks, 16);
		Akari->console->putString("\n");

		regs->useresp = USER_TASK_BASE;
		regs->ss = 0x10 + (cpl * 0x11);		// same as ds: ss is set by TSS, ds is manually set by irq_timer_multitask after
	} else {
		nt->ks = (u32)Akari->memory->allocAligned(USER_TASK_STACK_SIZE) + USER_TASK_STACK_SIZE;
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
	
#define _PROCESS_HEAP_START	0x0500000
#define _PROCESS_HEAP_SIZE	0x500000		// 5MiB
	for (u32 i = _PROCESS_HEAP_START; i < (_PROCESS_HEAP_START + _PROCESS_HEAP_SIZE); i += 0x1000)
		nt->pageDir->getPage(i, true)->allocAnyFrame(false, true);

	nt->heapStart = _PROCESS_HEAP_START;
	nt->heapEnd = nt->heapMax = _PROCESS_HEAP_START + _PROCESS_HEAP_SIZE;
	// Heap will be initialised first time we switch to the process.

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

Tasks::Task::Stream::Stream(): _wl_id(0) {
}

u32 Tasks::Task::Stream::registerWriter(bool exclusive) {
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

u32 Tasks::Task::Stream::registerListener() {
	u32 id = _nextId();
	_listeners.push_back(Listener(id));
	return id;
}

Tasks::Task::Stream::Listener &Tasks::Task::Stream::getListener(u32 id) {
	for (LinkedList<Listener>::iterator it = _listeners.begin(); it != _listeners.end(); ++it) {
		if (it->_id == id)
			return *it;
	}
	AkariPanic("No matching listener - TODO: something more useful.");
}

void Tasks::Task::Stream::writeAllListeners(const char *buffer, u32 n) {
	for (LinkedList<Listener>::iterator it = _listeners.begin(); it != _listeners.end(); ++it) {
		it->append(buffer, n);
	}
}

bool Tasks::Task::Stream::hasWriter(u32 id) const {
	for (LinkedList<u32>::iterator it = _writers.begin(); it != _writers.end(); ++it) {
		if (*it == id)
			return true;
	}
	return false;
}

bool Tasks::Task::Stream::hasListener(u32 id) const {
	for (LinkedList<Listener>::iterator it = _listeners.begin(); it != _listeners.end(); ++it) {
		if (it->_id == id)
			return true;
	}
	return false;
}

Tasks::Task::Stream::Listener::Listener(u32 id): _id(id), _buffer(0), _buflen(0), _hooked(0)
{ }

void Tasks::Task::Stream::Listener::append(const char *data, u32 n) {
	// HACK: hacky little appending string reallocing stupid buffer.
	// Write a proper appending buffer (with smarts) and refactor it later.
	if (n == 0) return;

	if (!_buffer) {
		_buflen = n;
		_buffer = new char[n];
		POSIX::memcpy(_buffer, data, n);
	} else {
		char *newbuf = new char[_buflen + n];
		POSIX::memcpy(newbuf, _buffer, _buflen);
		POSIX::memcpy(newbuf + _buflen, data, n);
		delete [] _buffer;
		_buffer = newbuf;
		_buflen += n;
	}

	if (_hooked) {
		_hooked->userWaiting = false;
	}
}

void Tasks::Task::Stream::Listener::reset() {
	if (_buffer) {
		delete [] _buffer;
		_buffer = 0;
		_buflen = 0;
	}
}

void Tasks::Task::Stream::Listener::cut(u32 n) {
	if (!_buffer) return;
	if (_buflen <= n) {
		delete [] _buffer;
		_buffer = 0;
		_buflen = 0;
	} else {
		// Not using memcpy since in the future we probably
		// will have no guarantee that it won't collapse if
		// using overlapping regions!
		char *rp = _buffer + n, *wp = _buffer;
		u32 rm = _buflen - n;
		while (rm-- > 0)
			*wp++ = *rp++;
		_buflen -= n;
	}
}

void Tasks::Task::Stream::Listener::hook(Task *task) {
	_hooked = task;
}

void Tasks::Task::Stream::Listener::unhook() {
	_hooked = 0;
}

const char *Tasks::Task::Stream::Listener::view() const {
	return _buffer;
}

u32 Tasks::Task::Stream::Listener::length() const {
	return _buflen;
}

u32 Tasks::Task::Stream::_nextId() {
	return ++_wl_id;
}

Tasks::Task::Queue::Queue() { }

Tasks::Task::Task(u8 cpl):
		next(0), priorityNext(0), irqWaiting(false), irqListen(0), irqListenHits(0),
		userWaiting(false), userCall(0),
		id(0), registeredName(),
		cpl(cpl), pageDir(0),
		heap(0), heapStart(0), heapEnd(0), heapMax(0),
		ks(0) {
	static u32 lastAssignedId = 0;	// wouldn't be surprised if this needs to be accessible some day
	id = ++lastAssignedId;

	for (u8 i = 0; i < 32; ++i)
		iomap[i] = 0xFF;

	streamsByName = new HashTable<Symbol, Stream *>();
	queuesByName = new HashTable<Symbol, Queue *>();
}
