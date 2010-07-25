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

#include <entry.hpp>
#include <debug.hpp>
#include <Akari.hpp>
#include <slist>
#include <UserGates.hpp>
#include <Tasks.hpp>
#include <Memory.hpp>
#include <Descriptor.hpp>
#include <Console.hpp>
#include <Timer.hpp>
#include <Syscall.hpp>
#include <ELF.hpp>
#include <Debugger.hpp>

#define UKERNEL_STACK_POS	0xE0000000
#define UKERNEL_STACK_SIZE	0x2000

#define INIT_STACK_SIZE		0x2000

static void AkariEntryCont();
void IdleProcess();

// Unused: just for linkage purposes, since a real __cxa_atexit might need it.
// (see below)
void *__dso_handle = (void *)&__dso_handle;

// Dummy __cxa_atexit: our kernel never exits, so we never need to call the
// destructors.
extern "C" int __cxa_atexit(void (*func)(void *), void *arg, void *dso_handle) {
	return 0;
}

multiboot_info_t *AkariMultiboot;

typedef struct {
	char *name;
	char *module;
	u32 module_len;
} loaded_module_t;

std::slist<loaded_module_t> modules;
static loaded_module_t *module_by_name(const char *name) {
	for (std::slist<loaded_module_t>::iterator it = modules.begin(); it != modules.end(); ++it) {
		if (stricmp(name, it->name) == 0)
			return &*it;
	}
	return 0;
}

void AkariEntry() {
	if ((AkariMultiboot->flags & 0x41) != 0x41)
		AkariPanic("Akari: MULTIBOOT hasn't given us enough information about memory.");

	Akari = Kernel::Construct(reinterpret_cast<u32>(&__kend), AkariMultiboot->mem_upper);
	Akari->subsystems.push_back(Akari->memory);

	// these can only work if Akari = an AkariKernel, since `new' calls Akari->...
	// how could we integrate these with construction in AkariKernel::Construct
	// without just doing it all by using placement new with kernel->Alloc?
	// (which is lame)
	Akari->console = new Console(); Akari->subsystems.push_back(Akari->console);
	Akari->descriptor = new Descriptor(); Akari->subsystems.push_back(Akari->descriptor);
	Akari->timer = new Timer(); Akari->subsystems.push_back(Akari->timer);
	Akari->tasks = new Tasks(); Akari->subsystems.push_back(Akari->tasks);
	Akari->syscall = new Syscall(); Akari->subsystems.push_back(Akari->syscall);
	Akari->elf = new ELF(); Akari->subsystems.push_back(Akari->elf);
	Akari->debugger = new Debugger(); Akari->subsystems.push_back(Akari->debugger);

	Akari->console->putString("Akari " __AKARI_VERSION ". Dedicerad till Misty.\nBörjar ...\n");

	// This is done before paging is turned on (otherwise the memory where the
	// modules reside is protected), but after memory is initialised.
	module_t *module_ptr = (module_t *)AkariMultiboot->mods_addr;
	u32 mods_count = AkariMultiboot->mods_count;
	while (mods_count--) {
		u32 len = module_ptr->mod_end - module_ptr->mod_start;

		loaded_module_t mod = {
			strdup(reinterpret_cast<const char *>(module_ptr->string)),
			static_cast<char *>(Akari->memory->alloc(len)),
			len
		};

		memcpy(mod.module, reinterpret_cast<const void *>(module_ptr->mod_start), len);

		modules.push_back(mod);
		++module_ptr;
	}

	Akari->timer->setTimer(100);
	Akari->memory->setPaging(true);
	
	// Give ourselves a normal stack. (n.b. this is from kernel heap!)
	void *initTaskStack = Akari->memory->allocAligned(INIT_STACK_SIZE);
	// do we still need to flush the TLB?
	__asm__ __volatile__("\
		mov %%cr3, %%eax; \
		mov %%eax, %%cr3" : : : "%eax");
	__asm__ __volatile__("\
		mov %%eax, %%esp; \
		mov %%eax, %%ebp" : : "a" (reinterpret_cast<u32>(initTaskStack)));
	
	AkariEntryCont();
}

static void AkariEntryCont() {	
	ASSERT(Akari->memory->_activeDirectory == Akari->memory->_kernelDirectory);
	for (u32 i = UKERNEL_STACK_POS; i >= UKERNEL_STACK_POS - UKERNEL_STACK_SIZE; i -= 0x1000)
		Akari->memory->_activeDirectory->getPage(i, true)->allocAnyFrame(false, true);

	// Initial task
	Tasks::Task *base = Tasks::Task::BootstrapInitialTask(3, Akari->memory->_kernelDirectory);
	Akari->tasks->start = Akari->tasks->current = base;

	Akari->descriptor->gdt->setTSSStack(base->ks + sizeof(struct modeswitch_registers));
	Akari->descriptor->gdt->setTSSIOMap(base->iomap);

	// Idle task
	Tasks::Task *idle = Tasks::Task::CreateTask(reinterpret_cast<u32>(&IdleProcess), 0, true, 0, Akari->memory->_kernelDirectory, "idle", std::list<std::string>());
	Akari->tasks->current->next = idle;

	// ATA driver
	Tasks::Task *ata = Tasks::Task::CreateTask(0, 3, true, 0, Akari->memory->_kernelDirectory, "ata", std::list<std::string>());
	Akari->elf->loadImageInto(ata, reinterpret_cast<u8 *>(module_by_name("/ata")->module));

	for (u16 j = 0; j < 8; ++j) {
		ata->setIOMap(0x1F0 + j, true);
		ata->setIOMap(0x170 + j, true);
	}

	ata->setIOMap(0x3F6, true);
	ata->setIOMap(0x376, true);

	// DMA busmastering. (assuming it'll be at c000. Need to do this later properly. XXX
	
	for (u16 j = 0; j < 16; ++j)
		ata->setIOMap(0xC000 + j, true);
	idle->next = ata;
	
	// FAT driver
	Tasks::Task *fat = Tasks::Task::CreateTask(0, 3, true, 0, Akari->memory->_kernelDirectory, "fat", std::list<std::string>());
	Akari->elf->loadImageInto(fat, reinterpret_cast<u8 *>(module_by_name("/fat")->module));
	ata->next = fat;
	
	// VFS driver
	Tasks::Task *vfs = Tasks::Task::CreateTask(0, 3, true, 0, Akari->memory->_kernelDirectory, "vfs", std::list<std::string>());
	Akari->elf->loadImageInto(vfs, reinterpret_cast<u8 *>(module_by_name("/vfs")->module));
	fat->next = vfs;
	
	// Booter
	Tasks::Task *booter = Tasks::Task::CreateTask(0, 3, true, 0, Akari->memory->_kernelDirectory, "booter", std::list<std::string>());
	Akari->elf->loadImageInto(booter, reinterpret_cast<u8 *>(module_by_name("/booter")->module));
	vfs->next = booter;
	
	// Now we need our own directory! BootstrapTask should've been nice enough to make us one anyway.
	Akari->memory->switchPageDirectory(base->pageDir);

	Akari->console->putString("Akari: operativsystemkärnainitialiseringen klar.\n");

	Tasks::SwitchRing(3, 0); // switches to ring 3, uses IOPL 0 (no I/O access unless iomap gives it) and enables interrupts.

	// We have a proper (kernel-mode) idle task we spawn above that hlts, so we
	// can exit, with an exit syscall. We can't actually call syscall_exit(), since
	// the function call would try to push to our stack, and we can't do that now
	// since we're in ring 3 and we have no write access!
	asm volatile("int $0x80" : : "a" (7));
}

void IdleProcess() {
	while (true) asm volatile("hlt");
}

// Returns how much the stack needs to be shifted.
void *AkariMicrokernel(struct modeswitch_registers *r) {
	// 100 times a second.
	Akari->timer->tick();
	Akari->tasks->saveRegisterToTask(Akari->tasks->current, r);
	Akari->tasks->cycleTask();
	return Akari->tasks->assignInternalTask(Akari->tasks->current);
}
