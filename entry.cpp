#include <entry.hpp>
#include <debug.hpp>
#include <Akari.hpp>

#define KERNEL_STACK_POS	0xE0000000
#define KERNEL_STACK_SIZE	0x2000

void AkariSetupStack(u32, u32);

multiboot_info_t *AkariMultiboot;
void *AkariStack;

void timer_func(struct registers r) {
	// Akari->Console->PutString("timer run\n");
}

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
	AkariSetupStack(KERNEL_STACK_POS, KERNEL_STACK_SIZE);

	__asm__ __volatile__("AkariEntryNewStack:");
	// Note that we cannot `make' new stack variables here, as they're actually
	// all part of the same stack which the C++ compiler thinks we had all along.
	// (imagine they're initialised at the start of the function).
	// We can't use old ones, either, as we're in a new stack from hereon.
	// Function calls work fine, though, as they all push onto the current stack.

	Akari->Console->PutString("\nSystem halted!");
	asm volatile("sti");
	while (1)
		asm volatile("hlt");
}

void AkariSetupStack(u32 start, u32 size) {
	// NOTE: this function returns to AkariEntryNewStack directly.

	for (u32 i = start; i >= start - size; i -= 0x1000)
		Akari->Memory->_activeDirectory->GetPage(i, true)->AllocAnyFrame(false, true);
	
	// flush transaction lookaside buffer
	__asm__ __volatile__("\
		mov %%cr3, %%eax; \
		mov %%eax, %%cr3" : : : "%eax");
	
	__asm__ __volatile__("\
		mov %%eax, %%esp; \
		mov %%eax, %%ebp; \
		jmp AkariEntryNewStack" : : "a" (start));
}

