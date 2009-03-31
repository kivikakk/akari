#include <entry.hpp>
#include <debug.hpp>
#include <Akari.hpp>

#define KERNEL_STACK_POS	0xE0000000
#define KERNEL_STACK_SIZE	0x2000

#define IDLE_STACK_SIZE		0x2000

static void AkariEntryCont();
void SubProcess();

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
	for (u32 i = KERNEL_STACK_POS; i >= KERNEL_STACK_POS - KERNEL_STACK_SIZE; i -= 0x1000)
		Akari->Memory->_activeDirectory->GetPage(i, true)->AllocAnyFrame(false, true);

	Akari->Descriptor->_gdt->SetTSSStack(KERNEL_STACK_POS);
	
	// esp, ebp, eip, usermode?, EFLAGS.IF
	// the only setting here which actually is important for the bootstrap task is `usermode' ...
	AkariTaskSubsystem::Task *base = AkariTaskSubsystem::Task::BootstrapTask(0, 0,
		0, true, true, Akari->Memory->_kernelDirectory);
	Akari->Task->start = Akari->Task->current = base;

	void *processStack = Akari->Memory->AllocAligned(0x2000);
	AkariTaskSubsystem::Task *other = AkariTaskSubsystem::Task::BootstrapTask((u32)processStack + 0x2000, (u32)processStack + 0x2000,
		(u32)&SubProcess, true, true, Akari->Memory->_kernelDirectory);
	Akari->Task->current->next = other;

	void *p2Stack = Akari->Memory->AllocAligned(0x2000);
	AkariTaskSubsystem::Task *third = AkariTaskSubsystem::Task::BootstrapTask((u32)p2Stack + 0x2000, (u32)p2Stack + 0x2000,
		(u32)&SubProcess, false, true, Akari->Memory->_kernelDirectory);
	other->next = third;

	Akari->Console->PutString("&SubProcess: &0x");
	Akari->Console->PutInt((u32)&SubProcess, 16);
	Akari->Console->PutString(".. doing sti.\n");

	// Now we need our own directory! BootstrapTask should've been nice enough to make us one anyway.
	Akari->Memory->SwitchPageDirectory(base->_pageDir);

	Akari->Task->SwitchToUsermode(); // enables interrupts

	SubProcess();
}

void SubProcessA(s32);

void SubProcess() { 
	u32 a = 0, b = 0;
	while (1) {
		// Something computationally differing so that interrupting at regular intervals
		// won't be at the same instruction.
		++a, --b;
		SubProcessA(1);
		if (a % 4 == 1) {
			a += 3;
			if (b % 2 == 1)
				--b;
		} else if (b % 7 == 2) {
			SubProcessA(4);
		}
	}
}

void SubProcessA(s32 n) {
	while (--n > 0) {
		__asm__ __volatile__("nop");
	}
}

// Returns how much the stack needs to be shifted.
struct modeswitch_registers *AkariMicrokernel(struct callback_registers *r) {
	Akari->Console->PutString("\nFrom: &1x");
	Akari->Console->PutInt(r->eip, 16);
	Akari->Console->PutString(", #");
	Akari->Console->PutInt(Akari->Task->current->_id, 16);
	Akari->Console->PutString(", EBP 0x");
	Akari->Console->PutInt(r->ebp, 16);
	Akari->Console->PutString(", ESP 0x");
	Akari->Console->PutInt(r->esp, 16);
	Akari->Console->PutString(", CS 0x");
	Akari->Console->PutInt(r->cs, 16);
	Akari->Console->PutString(", EFLAGS 0x");
	Akari->Console->PutInt(r->eflags, 16);
	if (Akari->Task->current->_userMode) {
		Akari->Console->PutString(", userESP 0x");
		Akari->Console->PutInt(((struct modeswitch_registers *)r)->useresp, 16);
		Akari->Console->PutString(", SS: 0x");
		Akari->Console->PutInt(((struct modeswitch_registers *)r)->ss, 16);
	}
	Akari->Console->PutString(", r at: 0x");
	Akari->Console->PutInt((u32)r, 16);

	if (Akari->Task->current->_userMode) {
		// modeswitch set of registers on stack
		Akari->Task->current->_registers = *((struct modeswitch_registers *)r);
	} else {
		// callback set only
		Akari->Task->current->_registers.callback = *r;
	}

	Akari->Task->current = Akari->Task->current->next ? Akari->Task->current->next : Akari->Task->start;
	Akari->Memory->SwitchPageDirectory(Akari->Task->current->_pageDir);

	if (!Akari->Task->current->_userMode) {
		// modify it right on the stack
		*r = Akari->Task->current->_registers.callback;
	}

	Akari->Console->PutString(".\nTo: &0x");
	Akari->Console->PutInt(r->eip, 16);
	Akari->Console->PutString(", #");
	Akari->Console->PutInt(Akari->Task->current->_id, 16);
	Akari->Console->PutString(", EBP 0x");
	Akari->Console->PutInt(r->ebp, 16);
	Akari->Console->PutString(", ESP 0x");
	Akari->Console->PutInt(r->esp, 16);
	Akari->Console->PutString(", CS 0x");
	Akari->Console->PutInt(r->cs, 16);
	Akari->Console->PutString(", EFLAGS 0x");
	Akari->Console->PutInt(r->eflags, 16);
	if (Akari->Task->current->_userMode) {
		Akari->Console->PutString(", userESP 0x");
		Akari->Console->PutInt(((struct modeswitch_registers *)r)->useresp, 16);
		Akari->Console->PutString(", SS: 0x");
		Akari->Console->PutInt(((struct modeswitch_registers *)r)->ss, 16);
	}
	Akari->Console->PutString(".\n");

	if (Akari->Task->current->_userMode) {
		// return the register address for irq_timer_multitask
		return &Akari->Task->current->_registers;
	}
	
	return 0;
}
