#include <entry.hpp>
#include <debug.hpp>
#include <Akari.hpp>

multiboot_info_t *AkariMultiboot;
void *AkariStack;

void timer_func(struct registers r) {
	// Akari->Console->PutString("timer run\n");
}

void AkariEntry() {
	if ((AkariMultiboot->flags & 0x41) != 0x41)
		AkariPanic("Akari: MULTIBOOT hasn't given us enough information about memory.");

	Akari = AkariKernel::Construct((u32)&__kend, AkariMultiboot->mem_upper);

	// these can only work if Akari = an AkariKernel, since `new' calls Akari-> ...
	// how could we integrate these with construction in AkariKernel::Construct
	// without just doing it all by using placement new with kernel->Alloc? (which is lame)
	Akari->Console = new AkariConsoleSubsystem();
	Akari->Descriptor = new AkariDescriptorSubsystem();
	Akari->Timer = new AkariTimerSubsystem();

	Akari->Timer->SetTimer(100);
	Akari->Descriptor->_irqt->InstallHandler(0, timer_func);

	Akari->Memory->SetPaging(true);

	u32 a = 0;
	u32 b = *((u32 *)a);

	Akari->Console->PutString("\nSystem halted!");
	asm volatile("sti");
	while (1)
		asm volatile("hlt");
}

