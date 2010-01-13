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
#include <UserGates.hpp>
#include <Tasks.hpp>
#include <Memory.hpp>
#include <Descriptor.hpp>
#include <Console.hpp>
#include <Timer.hpp>
#include <Syscall.hpp>

#define UKERNEL_STACK_POS	0xE0000000
#define UKERNEL_STACK_SIZE	0x2000

#define INIT_STACK_SIZE		0x2000

static void AkariEntryCont();
void SubProcess();

void KeyboardProcess();	// TmpKb
void ShellProcess();	// TmpShell
void ATAProcess();		// TmpATA
void IdleProcess();

multiboot_info_t *AkariMultiboot;

void AkariEntry() {
	if ((AkariMultiboot->flags & 0x41) != 0x41)
		AkariPanic("Akari: MULTIBOOT hasn't given us enough information about memory.");

	Akari = Kernel::Construct(reinterpret_cast<u32>(&__kend), AkariMultiboot->mem_upper);

	// these can only work if Akari = an AkariKernel, since `new' calls Akari->...
	// how could we integrate these with construction in AkariKernel::Construct
	// without just doing it all by using placement new with kernel->Alloc?
	// (which is lame)
	Akari->console = new Console();
	Akari->descriptor = new Descriptor();
	Akari->timer = new Timer();
	Akari->tasks = new Tasks();
	Akari->syscall = new Syscall();

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
	Tasks::Task *idle = Tasks::Task::CreateTask(reinterpret_cast<u32>(&IdleProcess), 0, true, 0, Akari->memory->_kernelDirectory);
	Akari->tasks->current->next = idle;

	// Keyboard driver task
	Tasks::Task *kbdriver = Tasks::Task::CreateTask(reinterpret_cast<u32>(&KeyboardProcess), 3, true, 0, Akari->memory->_kernelDirectory);
	kbdriver->setIOMap(0x60, true);
	kbdriver->setIOMap(0x64, true);
	idle->next = kbdriver;

	// Shell
	Tasks::Task *shell = Tasks::Task::CreateTask(reinterpret_cast<u32>(&ShellProcess), 3, true, 0, Akari->memory->_kernelDirectory);
	kbdriver->next = shell;
	
	// ATA driver
	Tasks::Task *ata = Tasks::Task::CreateTask(reinterpret_cast<u32>(&ATAProcess), 3, true, 0, Akari->memory->_kernelDirectory);
	ata->setIOMap(0x1F7, true);
	for (u16 j = 0; j < 8; ++j) {
		ata->setIOMap(0x1F0 + j, true);
		ata->setIOMap(0x170 + j, true);
	}
	ata->setIOMap(0x3F6, true);
	ata->setIOMap(0x376, true);
	shell->next = ata;
	
	// Now we need our own directory! BootstrapTask should've been nice enough to make us one anyway.
	Akari->memory->switchPageDirectory(base->pageDir);

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

u32 AkariMicrokernelSwitches = 0;

// Returns how much the stack needs to be shifted.
void *AkariMicrokernel(struct modeswitch_registers *r) {
	++AkariMicrokernelSwitches;
	Akari->tasks->saveRegisterToTask(Akari->tasks->current, r);
	Akari->tasks->cycleTask();
	return Akari->tasks->assignInternalTask(Akari->tasks->current);
}
