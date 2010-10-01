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
#include <UserIPC.hpp>

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
	std::list<std::string> *args;
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

	ptr_t addr = reinterpret_cast<ptr_t>(&__kend);
	mu_memory = new (reinterpret_cast<void *>(addr)) Memory(AkariMultiboot->mem_upper);
	addr += sizeof(Memory);
	mu_memory->setPlacementMode(addr);

	// these can only work if mu_memory is a Memory subsys, since `new' calls mu_memory->...
	mu_console = new Console();
	mu_descriptor = new Descriptor();
	mu_timer = new Timer();
	mu_tasks = new Tasks();
	mu_syscall = new Syscall();
	mu_elf = new ELF();
	mu_debugger = new Debugger();

	mu_console->putString("Akari " __AKARI_VERSION ". Dedicated to Misty.\nStarting ...\n");

	// This is done before paging is turned on (otherwise the memory where the
	// modules reside is protected), but after memory is initialised.
	module_t *module_ptr = (module_t *)AkariMultiboot->mods_addr;
	u32 mods_count = AkariMultiboot->mods_count;
	while (mods_count--) {
		u32 len = module_ptr->mod_end - module_ptr->mod_start;

		mu_console->putString("loaded module: ");
		mu_console->putString(reinterpret_cast<const char*>(module_ptr->string));
		mu_console->putString("\n");

		loaded_module_t mod = {
			strdup(reinterpret_cast<const char *>(module_ptr->string)),
			static_cast<char *>(mu_memory->alloc(len)),
			0,
			len
		};

		memcpy(mod.module, reinterpret_cast<const void *>(module_ptr->mod_start), len);

		modules.push_back(mod);
		++module_ptr;
	}

	mu_timer->setTimer(100);
	mu_memory->setPaging(true);
	
	// Give ourselves a normal stack. (n.b. this is from kernel heap!)
	void *initTaskStack = mu_memory->allocAligned(INIT_STACK_SIZE);
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
	ASSERT(mu_memory->_activeDirectory == mu_memory->_kernelDirectory);
	for (u32 i = UKERNEL_STACK_POS; i >= UKERNEL_STACK_POS - UKERNEL_STACK_SIZE; i -= 0x1000)
		mu_memory->_activeDirectory->getPage(i, true)->allocAnyFrame(false, true);

	// Now that we have something of a dynamic-y memory bit, let's look at the modules
	// and split away arguments.
	for (std::slist<loaded_module_t>::iterator it = modules.begin(); it != modules.end(); ++it) {
		std::string module = it->name;
		std::vector<std::string> args = module.split();

		if (args.size() > 1) {
			it->name = strdup(args[0].c_str());
			it->args = new std::list<std::string>();
			for (int i = 1; i < static_cast<int>(args.size()); ++i)
				it->args->push_back(args[i]);
		}
	}

	// Initial task
	Tasks::Task *base = Tasks::Task::BootstrapInitialTask(3, mu_memory->_kernelDirectory);
	mu_tasks->start = mu_tasks->current = base;

	mu_descriptor->gdt->setTSSStack(base->ks + sizeof(struct modeswitch_registers));
	mu_descriptor->gdt->setTSSIOMap(base->iomap);

	// Idle task
	Tasks::Task *idle = Tasks::Task::CreateTask(reinterpret_cast<u32>(&IdleProcess), 0, true, 0, mu_memory->_kernelDirectory, "idle", std::list<std::string>());
	mu_tasks->current->next = idle;

	// ATA driver
	loaded_module_t *module = module_by_name("/ata");
	ASSERT(module);
	Tasks::Task *ata = Tasks::Task::CreateTask(0, 3, true, 0, mu_memory->_kernelDirectory, "ata", module->args ? *module->args : std::list<std::string>());
	mu_elf->loadImageInto(ata, reinterpret_cast<u8 *>(module->module));
	mu_tasks->registeredTasks["system.io.ata"] = ata;
	ata->registeredName = "system.io.ata";

	for (u16 j = 0; j < 8; ++j) {
		ata->setIOMap(0x1F0 + j, true);
		ata->setIOMap(0x170 + j, true);
	}

	ata->setIOMap(0x3F6, true);
	ata->setIOMap(0x376, true);

	ata->grantPrivilege(PRIV_IRQ);
	ata->grantPrivilege(PRIV_PHYSADDR);

	// DMA busmastering. (assuming it'll be at c000. Need to do this later properly. XXX
	
	for (u16 j = 0; j < 16; ++j)
		ata->setIOMap(0xC000 + j, true);
	idle->next = ata;
	
	// FAT driver
	module = module_by_name("/fat");
	ASSERT(module);
	Tasks::Task *fat = Tasks::Task::CreateTask(0, 3, true, 0, mu_memory->_kernelDirectory, "fat", module->args ? *module->args : std::list<std::string>());
	mu_elf->loadImageInto(fat, reinterpret_cast<u8 *>(module->module));
	mu_tasks->registeredTasks["system.io.fs.fat"] = fat;
	fat->registeredName = "system.io.fs.fat";
	ata->next = fat;
	
	// VFS driver
	module = module_by_name("/vfs");
	ASSERT(module);
	Tasks::Task *vfs = Tasks::Task::CreateTask(0, 3, true, 0, mu_memory->_kernelDirectory, "vfs", module->args ? *module->args : std::list<std::string>());
	mu_elf->loadImageInto(vfs, reinterpret_cast<u8 *>(module->module));
	mu_tasks->registeredTasks["system.io.vfs"] = vfs;
	vfs->registeredName = "system.io.vfs";
	fat->next = vfs;
	
	// Booter
	module = module_by_name("/booter");
	ASSERT(module);
	Tasks::Task *booter = Tasks::Task::CreateTask(0, 3, true, 0, mu_memory->_kernelDirectory, "booter", module->args ? *module->args : std::list<std::string>());
	mu_elf->loadImageInto(booter, reinterpret_cast<u8 *>(module->module));
	booter->grantPrivilege(PRIV_REGISTER_NAME);
	booter->grantPrivilege(PRIV_GRANT_PRIV);
	vfs->next = booter;
	
	// Now we need our own directory! BootstrapTask should've been nice enough to make us one anyway.
	mu_memory->switchPageDirectory(base->pageDir);

	mu_console->putString("Akari: kernel initialised.\n");

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
	mu_timer->tick();
	mu_tasks->saveRegisterToTask(mu_tasks->current, r);
	mu_tasks->cycleTask();
	return mu_tasks->assignInternalTask(mu_tasks->current);
}
