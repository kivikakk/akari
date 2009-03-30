#include <entry.hpp>
#include <debug.hpp>
#include <Akari.hpp>

#define KERNEL_STACK_POS	0xE0000000
#define KERNEL_STACK_SIZE	0x2000

static void AkariEntryCont();
static void timer_func(struct registers *);

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
	Akari->Descriptor->_irqt->InstallHandler(0, timer_func);
	Akari->Memory->SetPaging(true);
	
	// here we switch the stack to somewhere predictable.. assume we don't need
	// any of the stack as it is

	ASSERT(Akari->Memory->_activeDirectory == Akari->Memory->_kernelDirectory);
	for (u32 i = KERNEL_STACK_POS; i >= KERNEL_STACK_POS - KERNEL_STACK_SIZE; i -= 0x1000)
		Akari->Memory->_activeDirectory->GetPage(i, true)->AllocAnyFrame(false, true);
	
	// flush translation lookaside buffer
	__asm__ __volatile__("\
		mov %%cr3, %%eax; \
		mov %%eax, %%cr3" : : : "%eax");
	
	__asm__ __volatile__("\
		mov %%eax, %%esp; \
		mov %%eax, %%ebp;" : : "a" (KERNEL_STACK_POS));

	AkariEntryCont();		// it's important to do this so we're in a new stack context
}

static void AkariEntryCont() {
	__asm__ __volatile__("\
			jmp 2f; \
		.globl subProcessEntryPoint; \
		subProcessEntryPoint:	\
			mov $0, %%eax; \
		1: \
			inc %%eax; \
			jmp 1b; \
		2:	" :);

	u32 esp, ebp;
	__asm__ __volatile__("\
		mov %%esp, %0; \
		mov %%ebp, %1" : "=r" (esp), "=r" (ebp));
	Akari->Console->PutString("esp is: 0x");
	Akari->Console->PutInt(esp, 16);
	Akari->Console->PutString("\nebp is: 0x");
	Akari->Console->PutInt(ebp, 16);
	Akari->Console->PutString("\n");

	u32 entryPoint;
	__asm__ __volatile__("\
		movl $subProcessEntryPoint, %0" : "=m" (entryPoint));

	Akari->Console->PutString("entry point: 0x");
	Akari->Console->PutInt(entryPoint, 16);
	Akari->Console->PutString("\n");

	ASSERT(Akari->Memory->_kernelDirectory == Akari->Memory->_activeDirectory);
	// esp, ebp, eip
	AkariTaskSubsystem::Task *base = AkariTaskSubsystem::Task::BootstrapTask(0, 0, 0, Akari->Memory->_kernelDirectory);
	Akari->Task->start = Akari->Task->current = base;

	void *processStack = Akari->Memory->AllocAligned(0x2000);
	AkariTaskSubsystem::Task *other = AkariTaskSubsystem::Task::BootstrapTask((u32)processStack + 0x2000, (u32)processStack + 0x2000, entryPoint, Akari->Memory->_kernelDirectory);
	Akari->Task->current->next = other;

	u32 a = 0, b = 0;
	Akari->Console->PutString("\ndoing sti\n");
	asm volatile("sti");
	while (1) {
		// Something computationally differing so that interrupting at regular intervals
		// won't be at the same instruction.
		++a, --b;
		if (a % 4 == 1) {
			a += 3;
			if (b % 2 == 1)
				--b;
		} else if (b % 7 == 2) {
			--a;
		}
	}
}

void timer_func(struct registers *r) {
	Akari->Console->PutString("\nExecuting EIP was ");
	Akari->Console->PutInt(r->eip, 16);
	Akari->Console->PutString(", task was ");
	Akari->Console->PutInt(Akari->Task->current->_id, 16);
	Akari->Console->PutString(", 'real' ESP ");
	Akari->Console->PutInt(r->esp, 16);
	Akari->Console->PutString(", user ESP ");
	Akari->Console->PutInt(r->useresp, 16);
	Akari->Console->PutString(", SS: ");
	Akari->Console->PutInt(r->ss, 16);

	Akari->Task->current->_registers = *r;
	Akari->Task->current = Akari->Task->current->next ? Akari->Task->current->next : Akari->Task->start;
	*r = Akari->Task->current->_registers;

	Akari->Memory->SwitchPageDirectory(Akari->Task->current->_pageDir);
	// this should be tenable, as all the kernel-space stuff should be linked

	Akari->Console->PutString(".\nSwitching to EIP ");
	Akari->Console->PutInt(r->eip, 16);
	Akari->Console->PutString(", task #");
	Akari->Console->PutInt(Akari->Task->current->_id, 16);
	Akari->Console->PutString(", 'real' ESP ");
	Akari->Console->PutInt(r->esp, 16);
	Akari->Console->PutString(", user ESP ");
	Akari->Console->PutInt(r->useresp, 16);
	Akari->Console->PutString(", SS: ");
	Akari->Console->PutInt(r->ss, 16);
	Akari->Console->PutString(".\n");
}
