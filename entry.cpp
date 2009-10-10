#include <entry.hpp>
#include <debug.hpp>
#include <Akari.hpp>
#include <UserGates.hpp>

#define UKERNEL_STACK_POS	0xE0000000
#define UKERNEL_STACK_SIZE	0x2000

#define IDLE_STACK_SIZE		0x2000

static void AkariEntryCont();
void SubProcess();

void KeyboardProcess();	// TmpKb
void ShellProcess();	// TmpShell
void IdleProcess();		// TmpIdle

multiboot_info_t *AkariMultiboot;

void AkariEntry() {
	if ((AkariMultiboot->flags & 0x41) != 0x41)
		AkariPanic("Akari: MULTIBOOT hasn't given us enough information about memory.");

	Akari = AkariKernel::Construct((u32)&__kend, AkariMultiboot->mem_upper);

	// these can only work if Akari = an AkariKernel, since `new' calls Akari->...
	// how could we integrate these with construction in AkariKernel::Construct
	// without just doing it all by using placement new with kernel->Alloc?
	// (which is lame)
	Akari->Console = new AkariConsoleSubsystem();
	Akari->Descriptor = new AkariDescriptorSubsystem();
	Akari->Timer = new AkariTimerSubsystem();
	Akari->Task = new AkariTaskSubsystem();
	Akari->Syscall = new AkariSyscallSubsystem();

	Akari->Timer->setTimer(100);
	Akari->Memory->setPaging(true);
	
	// Give ourselves a normal stack. (n.b. this is from kernel heap!)
	void *IdleTaskStack = Akari->Memory->allocAligned(IDLE_STACK_SIZE);
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
	ASSERT(Akari->Memory->_activeDirectory == Akari->Memory->_kernelDirectory);
	for (u32 i = UKERNEL_STACK_POS; i >= UKERNEL_STACK_POS - UKERNEL_STACK_SIZE; i -= 0x1000)
		Akari->Memory->_activeDirectory->getPage(i, true)->allocAnyFrame(false, true);

	AkariTaskSubsystem::Task *base = AkariTaskSubsystem::Task::BootstrapInitialTask(
		3, Akari->Memory->_kernelDirectory);
	Akari->Task->start = Akari->Task->current = base;

	Akari->Descriptor->gdt->setTSSStack(base->utks + sizeof(struct modeswitch_registers));
	Akari->Descriptor->gdt->setTSSIOMap(base->iomap);

	// Idle task
	AkariTaskSubsystem::Task *idle = AkariTaskSubsystem::Task::CreateTask(
		(u32)&IdleProcess, 0, true, 0, Akari->Memory->_kernelDirectory);
	Akari->Task->current->next = idle;

	// Keyboard driver task
	AkariTaskSubsystem::Task *kbdriver = AkariTaskSubsystem::Task::CreateTask(
		(u32)&KeyboardProcess, 1, true, 0, Akari->Memory->_kernelDirectory);
	kbdriver->setIOMap(0x60, true);
	kbdriver->setIOMap(0x64, true);
	idle->next = kbdriver;

	// Hello shell!
	AkariTaskSubsystem::Task *shell = AkariTaskSubsystem::Task::CreateTask(
		(u32)&ShellProcess, 0, true, 0, Akari->Memory->_kernelDirectory);
	kbdriver->next = shell;
	
	// Now we need our own directory! BootstrapTask should've been nice enough to make us one anyway.
	Akari->Memory->switchPageDirectory(base->pageDir);

	AkariTaskSubsystem::SwitchRing(3, 0); // switches to ring 3, uses IOPL 0 (no I/O access unless iomap gives it) and enables interrupts.

	// We have a proper (kernel-mode) idle task we spawn above that hlts.
	syscall_exit();
}

// Returns how much the stack needs to be shifted.
void *AkariMicrokernel(struct modeswitch_registers *r) {
	Akari->Task->saveRegisterToTask(Akari->Task->current, r);
	Akari->Task->cycleTask();
	return Akari->Task->assignInternalTask(Akari->Task->current);
}
