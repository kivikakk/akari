#include <entry.hpp>
#include <debug.hpp>
#include <Akari.hpp>

multiboot_info_t *AkariMultiboot;
void *AkariStack;

void timer_func(struct registers r) {
	Akari->Console->PutString("timer run\n");
}

void AkariEntry() {
	// Bootstrap memory subsystems; we'll start allocating from here.
	u32 placementAddress = (u32)&__kend;

	if ((AkariMultiboot->flags & 0x41) != 0x41)
		AkariPanic("Akari: MULTIBOOT hasn't given us enough information about memory.");

	Akari = new ((void *)placementAddress) AkariKernel();
	placementAddress += sizeof(AkariKernel);
	Akari->Memory = new ((void *)placementAddress) AkariMemorySubsystem(AkariMultiboot->mem_upper);
	placementAddress += sizeof(AkariMemorySubsystem);

	// TODO: move much of this into Akari's initialisation code
	Akari->Memory->SetPlacementMode(placementAddress);
	// we can now use `new' to use the memory manager's placement

	Akari->Console = new AkariConsoleSubsystem();
	Akari->Descriptor = new AkariDescriptorSubsystem();
	Akari->Timer = new AkariTimerSubsystem();

	Akari->Timer->SetTimer(100);
	Akari->Descriptor->_irqt->InstallHandler(0, timer_func);

	for (u8 i = 0; i < 0x30; ++i) {
		Akari->Console->PutInt(i, 16);
		Akari->Console->PutChar(' ');
	}

	Akari->Console->PutString("\nSystem halted!");
	asm volatile("sti");
	while (1)
		asm volatile("hlt");
}

