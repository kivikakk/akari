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

#define IDLE_STACK_SIZE		0x2000

static void AkariEntryCont();
void SubProcess();

void KeyboardProcess();	// TmpKb
void ShellProcess();	// TmpShell
void IdleProcess();

multiboot_info_t *AkariMultiboot;

void AkariEntry() {
	if ((AkariMultiboot->flags & 0x41) != 0x41)
		AkariPanic("Akari: MULTIBOOT hasn't given us enough information about memory.");

	Akari = Kernel::Construct((u32)&__kend, AkariMultiboot->mem_upper);

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
	void *IdleTaskStack = Akari->memory->allocAligned(IDLE_STACK_SIZE);
	// do we still need to flush the TLB?
	__asm__ __volatile__("\
		mov %%cr3, %%eax; \
		mov %%eax, %%cr3" : : : "%eax");
	__asm__ __volatile__("\
		mov %%eax, %%esp; \
		mov %%eax, %%ebp" : : "a" ((u32)IdleTaskStack));
	
	AkariEntryCont();
}

static void AkariEntryCont() {	
	ASSERT(Akari->memory->_activeDirectory == Akari->memory->_kernelDirectory);
	for (u32 i = UKERNEL_STACK_POS; i >= UKERNEL_STACK_POS - UKERNEL_STACK_SIZE; i -= 0x1000)
		Akari->memory->_activeDirectory->getPage(i, true)->allocAnyFrame(false, true);

	Tasks::Task *base = Tasks::Task::BootstrapInitialTask(
		3, Akari->memory->_kernelDirectory);
	Akari->tasks->start = Akari->tasks->current = base;

	Akari->descriptor->gdt->setTSSStack(base->utks + sizeof(struct modeswitch_registers));
	Akari->descriptor->gdt->setTSSIOMap(base->iomap);

	// Idle task
	Tasks::Task *idle = Tasks::Task::CreateTask(
		(u32)&IdleProcess, 0, true, 0, Akari->memory->_kernelDirectory);
	Akari->tasks->current->next = idle;

	// Keyboard driver task
	Tasks::Task *kbdriver = Tasks::Task::CreateTask(
		(u32)&KeyboardProcess, 1, true, 0, Akari->memory->_kernelDirectory);
	kbdriver->setIOMap(0x60, true);
	kbdriver->setIOMap(0x64, true);
	idle->next = kbdriver;

	// Hello shell!
	Tasks::Task *shell = Tasks::Task::CreateTask(
		(u32)&ShellProcess, 3, true, 0, Akari->memory->_kernelDirectory);
	kbdriver->next = shell;
	
	// Now we need our own directory! BootstrapTask should've been nice enough to make us one anyway.
	Akari->memory->switchPageDirectory(base->pageDir);

	Tasks::SwitchRing(3, 0); // switches to ring 3, uses IOPL 0 (no I/O access unless iomap gives it) and enables interrupts.

	// We have a proper (kernel-mode) idle task we spawn above that hlts.
	syscall_exit();
}

void IdleProcess() {
	while (true) asm volatile("hlt");
}

// Returns how much the stack needs to be shifted.
void *AkariMicrokernel(struct modeswitch_registers *r) {
	Akari->tasks->saveRegisterToTask(Akari->tasks->current, r);
	Akari->tasks->cycleTask();
	return Akari->tasks->assignInternalTask(Akari->tasks->current);
}
