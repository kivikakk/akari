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

	Akari->Timer->SetTimer(100);
	Akari->Memory->SetPaging(true);
	
	// Give ourselves a normal stack. (n.b. this is from kernel heap!)
	void *IdleTaskStack = Akari->Memory->AllocAligned(IDLE_STACK_SIZE);
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
		Akari->Memory->_activeDirectory->GetPage(i, true)->AllocAnyFrame(false, true);

	AkariTaskSubsystem::Task *base = AkariTaskSubsystem::Task::BootstrapInitialTask(
		3, Akari->Memory->_kernelDirectory);
	Akari->Task->start = Akari->Task->current = base;

	Akari->Descriptor->_gdt->SetTSSStack(base->_utks + sizeof(struct modeswitch_registers));
	Akari->Descriptor->_gdt->SetTSSIOMap(base->_iomap);

	// Keyboard driver task
	AkariTaskSubsystem::Task *kbdriver = AkariTaskSubsystem::Task::CreateTask(
		(u32)&KeyboardProcess, 1, true, 0, Akari->Memory->_kernelDirectory);
	kbdriver->SetIOMap(0x60, true);
	kbdriver->SetIOMap(0x64, true);
	Akari->Task->current->next = kbdriver;

	// another usermode task
	AkariTaskSubsystem::Task *other = AkariTaskSubsystem::Task::CreateTask(
		(u32)&SubProcess, 3, true, 0, Akari->Memory->_kernelDirectory);
	kbdriver->next = other;
	
	// kernel-mode task
	AkariTaskSubsystem::Task *third = AkariTaskSubsystem::Task::CreateTask(
		(u32)&SubProcess, 0, true, 0, Akari->Memory->_kernelDirectory);
	other->next = third;

	// Now we need our own directory! BootstrapTask should've been nice enough to make us one anyway.
	Akari->Memory->SwitchPageDirectory(base->_pageDir);

	Akari->Task->SwitchRing(3, 0); // switches to ring 3, uses IOPL 0 (no I/O access unless iomap gives it) and enables interrupts.

	SubProcess();
}

void SubProcess() { 
	u32 a = 0, b = 0;
	while (1) {
		// Something computationally differing so that interrupting at regular intervals
		// won't be at the same instruction.

		// syscall_putl(syscall_getProcessId(), 16);
		// syscall_puts(" ");

		++a, --b;
		if (a % 4 == 1) {
			a += 3;
			if (b % 2 == 1)
				--b;
		}
	}
}

// Returns how much the stack needs to be shifted.
void *AkariMicrokernel(struct modeswitch_registers *r) {
	return Akari->Task->CycleTask(r);
}
