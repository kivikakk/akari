// This file is part of Akari.
// Copyright 2010 Arlen Cuss
//
// Akari is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Akari is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Akari.  If not, see <http://www.gnu.org/licenses/>.

#include <Timer.hpp>
#include <Tasks.hpp>
#include <Akari.hpp>
#include <Descriptor.hpp>
#include <Console.hpp>
#include <UserIPC.hpp>
#include <physmem.hpp>
#include <ELFInternal.hpp>
#include <interrupts.hpp>

Tasks::Tasks(): start(0), current(0), priorityStart(0)
{ }

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

Tasks::Task *Tasks::getTaskById(pid_t id) const {
	Task *const *read = &start;
	while (*read) {
		if ((*read)->id == id)
			return *read;
		read = &(*read)->next;
	}
	return 0;
}

static inline Tasks::Task *_NextTask(Tasks::Task *t) {
	return t->next ? t->next : Akari->tasks->start;
}

Tasks::Task *Tasks::prepareFetchNextTask() {
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

		if (t->userWaiting) {
			Task *newCurrent = t;
			while (newCurrent->userWaiting) {
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
	current = prepareFetchNextTask();
}

void Tasks::saveRegisterToTask(Task *dest, void *regs) {
	// we're saving a task even though it's in a state not matching its CPL?
	// This could happen if, say, it's in kernel mode, then interrupts or something.
	// HACK: need to handle this situation properly, but for now, ensure
	// internal consistency.
	ASSERT(((static_cast<struct modeswitch_registers *>(regs)->callback.cs - 0x08) / 0x11) == dest->cpl);

	// update the tip of stack pointer so we can restore later
	dest->ks = reinterpret_cast<u32>(regs);
}

void *Tasks::assignInternalTask(Task *task) {
	// now set the page directory, ks for TSS (if applicable) and stack to switch to as appropriate
	
	ASSERT(task);
	Akari->memory->switchPageDirectory(task->pageDir);

	if (!task->heap && task->heapStart != task->heapMax) {
		task->heap = new Heap(task->heapStart, task->heapEnd, task->heapMax, PROC_HEAP_SIZE, false, false);	// false false? XXX Always?
	}

	if (task->cpl > 0) {
		Akari->descriptor->gdt->setTSSStack(task->ks + sizeof(struct modeswitch_registers));
		Akari->descriptor->gdt->setTSSIOMap(task->iomap);
	}

	if (task->userWaiting) {
		AkariPanic("task->userWaiting");			// We shouldn't be switching to a task that's waiting. Block fail?
	} else if (task->userCall) {
		// We want to change the value in the stack which actually becomes the return value of the syscall.
		// If they're a kernel proc (cpl==0), then that's just the EAX on the ks.
		// // XXX if they aren't??
		struct modeswitch_registers *regs = reinterpret_cast<struct modeswitch_registers *>(task->ks);
		regs->callback.eax = (*task->userCall)();
		ASSERT(!task->userCall->shallBlock());

		delete task->userCall;
		task->userCall = 0;
	}

	return reinterpret_cast<void *>(task->ks);
}

Tasks::Task *Tasks::Task::BootstrapInitialTask(u8 cpl, Memory::PageDirectory *pageDirBase) {
	Task *nt = new Task(cpl, "initial task");

	if (cpl > 0)
		nt->ks = reinterpret_cast<u32>(Akari->memory->allocAligned(USER_TASK_KERNEL_STACK_SIZE)) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);
	else {
		// nt->ks = reinterpret_cast<u32>(Akari->memory->allocAligned(USER_TASK_KERNEL_STACK_SIZE)) + USER_TASK_KERNEL_STACK_SIZE - sizeof(struct modeswitch_registers);
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

static u32 load_task_argv(Tasks::Task *nt, const std::list<std::string> &args) {
	u32 total_args_len = 0;
	for (std::list<std::string>::iterator it = args.begin(); it != args.end(); ++it)
		total_args_len += it->length() + 1;	// plus a NUL

	u32 base = total_args_len + 4 * (args.size() + 4);

	u32 start_phys;
	u8 *start_page = static_cast<u8 *>(Akari->memory->allocAligned(0x1000, &start_phys));
	*reinterpret_cast<u32 *>(start_page + 0x1000 - base) = 0xDEADBEEF;

	*reinterpret_cast<u32 *>(start_page + 0x1000 - base + 4) = args.size();
	*reinterpret_cast<u32 *>(start_page + 0x1000 - base + 8) = USER_TASK_BASE - base + 12; // char **argv

	int arr_i = 0, str_i = 0;
	for (std::list<std::string>::iterator it = args.begin(); it != args.end(); ++it) {
		*reinterpret_cast<u32 *>(start_page + 0x1000 - base + 12 + (arr_i * 4)) =
			USER_TASK_BASE - base + 12 + ((args.size() + 1) * 4) + str_i; // char *argv[arr_i]

		strcpy(reinterpret_cast<char *>(start_page + 0x1000 - base + 12 + ((args.size() + 1) * 4) + str_i), it->c_str());
		
		++arr_i;
		str_i += it->length() + 1;
	}
	*reinterpret_cast<u32 *>(start_page + 0x1000 - base + 12 + args.size() * 4) = 0;

	AkariCopyFramePhysical(start_phys, nt->pageDir->getPage(USER_TASK_BASE - 0x1000, true)->pageAddress * 0x1000);

	Akari->memory->free(start_page);

	return base;
}

Tasks::Task *Tasks::Task::CreateTask(u32 entry, u8 cpl, bool interruptFlag, u8 iopl, Memory::PageDirectory *pageDirBase, const char *name, const std::list<std::string> &args) {
	Task *nt = new Task(cpl, name);

	nt->pageDir = pageDirBase->clone();

	struct modeswitch_registers *regs = 0;

	if (cpl > 0) {
		// Allocate the user's stack to top out at USER_TASK_BASE (i.e. it goes below that).
		// We need to do the frame allocs ourselves so they're visible and writable by the user.
		for (u32 i = 0, page = USER_TASK_BASE - 0x1000; i < USER_TASK_STACK_SIZE; i += 0x1000, page -= 0x1000) {
			nt->pageDir->getPage(page, true)->allocAnyFrame(false, true);
		}

		nt->ks = reinterpret_cast<u32>(Akari->memory->allocAligned(USER_TASK_KERNEL_STACK_SIZE))
			+ USER_TASK_KERNEL_STACK_SIZE
			- sizeof(struct modeswitch_registers);
		regs = reinterpret_cast<struct modeswitch_registers *>(nt->ks);

		u32 base = load_task_argv(nt, args);

		regs->useresp = USER_TASK_BASE - base;		// argc, argv, and the EIP expected to be there(?)
		regs->ss = 0x10 + (cpl * 0x11);		// same as ds: ss is set by TSS, ds is manually set by irq_timer_multitask after

	} else {
		nt->ks = reinterpret_cast<u32>(Akari->memory->allocAligned(USER_TASK_STACK_SIZE)) + USER_TASK_STACK_SIZE;
		nt->ks -= sizeof(struct callback_registers);
		regs = reinterpret_cast<struct modeswitch_registers *>(nt->ks);
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
	
	for (u32 i = PROCESS_HEAP_START; i < (PROCESS_HEAP_START + PROCESS_HEAP_SIZE); i += 0x1000) {
		nt->pageDir->getPage(i, true)->allocAnyFrame(false, true);
	}

	nt->heapStart = PROCESS_HEAP_START;
	nt->heapEnd = nt->heapMax = PROCESS_HEAP_START + PROCESS_HEAP_SIZE;
	// Heap will be initialised first time we switch to the process.

	return nt;
}

bool Tasks::Task::getIOMap(u16 port) const {
	return (iomap[port / 8] & (1 << (port % 8))) == 0;
}

void Tasks::Task::setIOMap(u16 port, bool enabled) {
	if (enabled)
		iomap[port / 8] &= ~(1 << (port % 8));
	else
		iomap[port / 8] |= (1 << (port % 8));
}

void Tasks::Task::unblockType(const Symbol &type) {
	if (userWaiting && userCall && userCall->insttype() == type) {
		userWaiting = false;
	}
}

void Tasks::Task::unblockTypeWith(const Symbol &type, u32 data) {
	if (userWaiting && userCall && userCall->insttype() == type) {
		if (userCall->unblockWith(data)) {
			userWaiting = false;
		}
	}
}

u8 *Tasks::Task::dumpELFCore(u32 *size) const {
	// Identify contiguous areas of allocated memory in the process's directory.
	std::list<u32> runs;
	
	u32 page_count = 0, run_count = 0;
	for (int pti = 0; pti < 1024; ++pti) {
		Memory::PageTable *pt = pageDir->tables[pti];
		if (!pt) continue;

		// Skip kernel linked tables.
		if  (Akari->memory->_kernelDirectory->tables[pti] == pt) continue;

		s32 last_start = -1;

		for (int pi = 0; pi < 1024; ++pi) {
			Memory::Page *p = &pt->pages[pi];

			if (last_start != -1 && !p->present) {
				// Run finished.
				++run_count;
				last_start = -1;
			} else if (last_start == -1 && p->present) {
				// New run.
				++page_count;
				last_start = pi;
				runs.push_back((u32)pti << 16 | (u32)pi);
			} else if (p->present) {
				++page_count;
			}
		}

		if (last_start != -1) {
			// Run finished.
			++run_count;
		}
	}

	// We'll put together the notes now.
	//
	// First find its size out; both the actual data sections,
	// as well as the 2 headers and the name.  Align after the
	// name and after the data sections.  We can assume we're
	// aligned to a dword already ...
	struct elf_prstatus prstatus;
	struct elf_prpsinfo prpsinfo;

	u32 notes_size = sizeof(prstatus);
	notes_size = (notes_size + 4 - 1) / 4 * 4;
	notes_size += sizeof(prpsinfo);
	notes_size = (notes_size + 4 - 1) / 4 * 4;
	notes_size += sizeof(Elf32_Nhdr) * 2 + (5 + 3) * 2;

	u8 *notes = new u8[notes_size];
	u8 *notes_write = notes;

	// PRSTATUS
	memset(&prstatus, 0, sizeof(struct elf_prstatus));

	struct modeswitch_registers *regs = reinterpret_cast<modeswitch_registers *>(ks);
	prstatus.pr_reg[0] = regs->callback.ebx;
	prstatus.pr_reg[1] = regs->callback.ecx;
	prstatus.pr_reg[2] = regs->callback.edx;
	prstatus.pr_reg[3] = regs->callback.esi;
	prstatus.pr_reg[4] = regs->callback.edi;
	prstatus.pr_reg[5] = regs->callback.ebp;
	prstatus.pr_reg[6] = regs->callback.eax;
	prstatus.pr_reg[7] = regs->callback.ds;
	prstatus.pr_reg[8] = regs->callback.ds;		// es
	prstatus.pr_reg[9] = regs->callback.ds;		// fs
	prstatus.pr_reg[10] = regs->callback.ds;	// gs
	prstatus.pr_reg[11] = regs->callback.eax;	// "orig ax"
	prstatus.pr_reg[12] = regs->callback.eip;
	prstatus.pr_reg[13] = regs->callback.cs;
	prstatus.pr_reg[14] = regs->callback.eflags;
	prstatus.pr_reg[15] = regs->useresp;
	prstatus.pr_reg[16] = regs->ss;
	
	Elf32_Nhdr *nhdr = reinterpret_cast<Elf32_Nhdr *>(notes_write);
	nhdr->n_namesz = 5;		// "CORE" + NUL
	nhdr->n_descsz = sizeof(struct elf_prstatus);
	nhdr->n_type = NT_PRSTATUS;

	notes_write += sizeof(Elf32_Nhdr);
	memcpy(notes_write, "CORE", 5);
	notes_write += 5;
	if ((notes_write - notes) % 4 != 0)
		notes_write += (4 - (notes_write - notes) % 4);
	
	memcpy(notes_write, &prstatus, sizeof(prstatus));
	notes_write += sizeof(prstatus);
	if ((notes_write - notes) % 4 != 0)
		notes_write += (4 - (notes_write - notes) % 4);

	// PRPSINFO
	memset(&prpsinfo, 0, sizeof(struct elf_prpsinfo));
	strncpy(prpsinfo.pr_fname, name.c_str(), 16);
	prpsinfo.pr_fname[15] = 0;

	nhdr = reinterpret_cast<Elf32_Nhdr *>(notes_write);
	nhdr->n_namesz = 5;
	nhdr->n_descsz = sizeof(elf_prpsinfo);
	nhdr->n_type = NT_PRPSINFO;

	notes_write += sizeof(Elf32_Nhdr);
	memcpy(notes_write, "CORE", 5);
	notes_write += 5;
	if ((notes_write - notes) % 4 != 0)
		notes_write += (4 - (notes_write - notes) % 4);

	memcpy(notes_write, &prpsinfo, sizeof(prpsinfo));
	notes_write += sizeof(struct elf_prpsinfo);
	if ((notes_write - notes) % 4 != 0)
		notes_write += (4 - (notes_write - notes) % 4);

	// AUXV (?)
	
	/*
	nhdr = reinterpret_cast<Elf32_Nhdr *>(notes_write);
	nhdr->n_namesz = 5;
	nhdr->n_descsz = SIZE;
	nhdr->n_type = NT_AUXV;

	notes_write += sizeof(Elf32_Nhdr);
	memcpy(notes_write, "CORE", 5);
	notes_write += 5;
	notes_write = (notes_write + 4 - 1) / 4 * 4;

	auxv_note?
	memcpy(notes_write, DATA, SIZE);
	notes_write += SIZE;
	notes_write = (notes_write + 4 - 1) / 4 * 4;
	*/

	Akari->console->printf("notes offset: %x\n", notes_write - notes);
	Akari->console->printf("notes approx: %x\n", notes_size);
	ASSERT(notes_write - notes == (s32)notes_size);

	*size = sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr) * (1 + run_count) + 0x1000 * page_count + notes_size;

	Akari->console->printf("Found %d run(s) in %d page(s)\n", run_count, page_count);
	Akari->console->printf("Allocating 0x%x bytes\n", *size);

	ptr_t helperphys;
	u8 *helper = reinterpret_cast<u8 *>(Akari->memory->alloc(0x1000, &helperphys));
	u8 *core = new u8[*size];

	Elf32_Ehdr *hdr = reinterpret_cast<Elf32_Ehdr *>(core);
	hdr->e_ident[EI_MAG0] = 0x7F;
	hdr->e_ident[EI_MAG1] = 'E';
	hdr->e_ident[EI_MAG2] = 'L';
	hdr->e_ident[EI_MAG3] = 'F';
	hdr->e_ident[EI_CLASS] = ELFCLASS32;
	hdr->e_ident[EI_DATA] = ELFDATA2LSB;
	hdr->e_ident[EI_VERSION] = EV_CURRENT;
	hdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
	hdr->e_ident[EI_ABIVERSION] = 0;
	for (int i = EI_PAD; i < EI_NIDENT; ++i)
		hdr->e_ident[i] = 0;

	hdr->e_type = ET_CORE;
	hdr->e_machine = EM_386;
	hdr->e_version = EV_CURRENT;
	hdr->e_entry = 0;

	hdr->e_phoff = sizeof(Elf32_Ehdr);
	hdr->e_shoff = 0;

	hdr->e_flags = 0;
	hdr->e_ehsize = sizeof(Elf32_Ehdr);

	hdr->e_phentsize = sizeof(Elf32_Phdr);
	hdr->e_phnum = 1 + run_count;

	hdr->e_shentsize = sizeof(Elf32_Shdr);
	hdr->e_shnum = 0;

	hdr->e_shstrndx = SHN_UNDEF;

	Elf32_Phdr *nextWriteHdr = reinterpret_cast<Elf32_Phdr *>(reinterpret_cast<u32>(core) + sizeof(Elf32_Ehdr));
   	u8 *nextWrite = reinterpret_cast<u8 *>(nextWriteHdr) + sizeof(Elf32_Phdr) * (1 + run_count);

	{
		nextWriteHdr->p_type = PT_NOTE;
		nextWriteHdr->p_offset = reinterpret_cast<u32>(nextWrite) - reinterpret_cast<u32>(core);
		nextWriteHdr->p_vaddr = nextWriteHdr->p_paddr = 0;

		memcpy(nextWrite, notes, notes_size);
		nextWrite += notes_size;
		
		nextWriteHdr->p_filesz = notes_size;
		nextWriteHdr->p_memsz = 0;

		nextWriteHdr->p_flags = 0;
		nextWriteHdr->p_align = 0;
		++nextWriteHdr;
	}

	for (std::list<u32>::iterator it = runs.begin(); it != runs.end(); ++it) {
		u32 pti = *it >> 16, pi = *it & 0xFFFF;
		Memory::PageTable *pt = pageDir->tables[pti];

		nextWriteHdr->p_type = PT_LOAD;
		nextWriteHdr->p_offset = reinterpret_cast<u32>(nextWrite) - reinterpret_cast<u32>(core);
		nextWriteHdr->p_vaddr = (pti * 1024 + pi) * 0x1000;
		nextWriteHdr->p_paddr = 0;

		u32 written = 0;
		for (; pi < 1024 && pt->pages[pi].present; ++pi) {
			AkariCopyFramePhysical(pt->pages[pi].pageAddress * 0x1000, helperphys);
			memcpy(nextWrite, helper, 0x1000);

			nextWrite += 0x1000;
			written += 0x1000;
		}

		nextWriteHdr->p_filesz = nextWriteHdr->p_memsz = written;
		nextWriteHdr->p_flags = PF_X | PF_W | PF_R;
		nextWriteHdr->p_align = 0x1000;

		Akari->console->printf("Phdr: %x - %x\n", nextWriteHdr->p_vaddr, nextWriteHdr->p_vaddr + nextWriteHdr->p_filesz);
		++nextWriteHdr;
	}

	delete [] helper;

	return core;
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
	for (std::list<Listener>::iterator it = _listeners.begin(); it != _listeners.end(); ++it) {
		if (it->_id == id)
			return *it;
	}
	AkariPanic("No matching listener - TODO: something more useful.");
}

void Tasks::Task::Stream::writeAllListeners(const char *buffer, u32 n) {
	for (std::list<Listener>::iterator it = _listeners.begin(); it != _listeners.end(); ++it) {
		it->append(buffer, n);
	}
}

bool Tasks::Task::Stream::hasWriter(u32 id) const {
	for (std::list<u32>::iterator it = _writers.begin(); it != _writers.end(); ++it) {
		if (*it == id)
			return true;
	}
	return false;
}

bool Tasks::Task::Stream::hasListener(u32 id) const {
	for (std::list<Listener>::iterator it = _listeners.begin(); it != _listeners.end(); ++it) {
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
		memcpy(_buffer, data, n);
	} else {
		char *newbuf = new char[_buflen + n];
		memcpy(newbuf, _buffer, _buflen);
		memcpy(newbuf + _buflen, data, n);
		delete [] _buffer;
		_buffer = newbuf;
		_buflen += n;
	}

	if (_hooked) {
		_hooked->unblockType(User::IPC::ReadStreamCall::type());
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

Tasks::Task::Queue::Item::Item(u32 id, u32 timestamp, pid_t from, u32 reply_to, const void *_data, u32 data_len):
	data(0)
{
	info.id = id;
	info.timestamp = timestamp;
	info.from = from;
	info.reply_to = reply_to;
	info.data_len = data_len;

	data = new char[data_len];
	memcpy(data, _data, data_len);
}

Tasks::Task::Queue::Item::~Item() {
	if (data)
		delete [] data;
}

Tasks::Task::Queue::Queue() { }

u32 Tasks::Task::Queue::push_back(pid_t from, u32 reply_to, const void *data, u32 data_len) {
	// This should become some random-ish guid in the future.
	// I don't like these being guessable.
	static u32 last_msg_id = 0;		

	Item *item = new Item(++last_msg_id, AkariMicrokernelSwitches, from, reply_to, data, data_len);
	list.push_back(item);
	return last_msg_id;
}

void Tasks::Task::Queue::shift() {
	if (list.empty()) return;
	delete *list.begin();
	list.pop_front();
}

void Tasks::Task::Queue::remove(Item *item) {
	list.remove(item);
}

Tasks::Task::Queue::Item *Tasks::Task::Queue::first() {
	if (list.empty()) return 0;
	return *list.begin();
}

Tasks::Task::Queue::Item *Tasks::Task::Queue::itemByReplyTo(u32 reply_to) {
	for (std::list<Tasks::Task::Queue::Item *>::iterator it = list.begin(); it != list.end(); ++it) {
		if ((*it)->info.reply_to == reply_to)
			return *it;
	}
	return 0;
}

Tasks::Task::Queue::Item *Tasks::Task::Queue::itemById(u32 id) {
	for (std::list<Tasks::Task::Queue::Item *>::iterator it = list.begin(); it != list.end(); ++it) {
		if ((*it)->info.id == id)
			return *it;
	}
	return 0;
}

Tasks::Task::Task(u8 cpl, const std::string &name):
		next(0), priorityNext(0), irqListen(0), irqListenHits(0), irqWaitStart(0),
		userWaiting(false), userCall(0),
		id(0), name(name), registeredName(),
		cpl(cpl), pageDir(0),
		heap(0), heapStart(0), heapEnd(0), heapMax(0),
		ks(0) {
	static pid_t lastAssignedId = 0;	// wouldn't be surprised if this needs to be accessible some day
	id = ++lastAssignedId;

	for (u16 i = 0; i < 8192; ++i)
		iomap[i] = 0xFF;

	streamsByName = new std::map<Symbol, Stream *>();
	replyQueue = new Queue();
}

Tasks::Task::~Task() {
	if (userCall) delete userCall;
	// TODO: destroy pageDir? the actual space allocated within the heap?
	// TODO: what about payloads in replyQueue?
	
	// TODO: do we need to deallocate the Stream*s within streamsByName?
	// Good chance we do.
	delete streamsByName;
	delete replyQueue;
}
