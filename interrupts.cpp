#include <Akari.hpp>
#include <interrupts.hpp>

static const char *isr_messages[] = {
    "Division by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Into detected overflow",
    "Out of bounds",
    "Invalid opcode",
    "No coprocessor",
    "Double fault",
    "Coprocessor segment overrun",
    "Bad TSS",
    "Segment not present",
    "Stack fault",
    "General protection fault",
    "Page fault",
    "Unknown interrupt",
    "Coprocessor fault",
    "Alignment check",
    "Machine check",
    /* reserved exceptions are empty */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void *isr_handler(struct modeswitch_registers *r) {
	ASSERT(r->callback.int_no < 0x20 || r->callback.int_no >= 0x30);
	// I guess this is right? What other IDTs are there? Hm.

	void *resume = Akari->Descriptor->idt->CallHandler(r->callback.int_no, r);
	if (!resume) {
		// nothing to call!
		Akari->Console->PutChar('\n');
		Akari->Console->PutString((r->callback.int_no < 32 && isr_messages[r->callback.int_no]) ? isr_messages[r->callback.int_no] : "[Intel reserved]");
		Akari->Console->PutString(" exception occured!\n");

		Akari->Console->PutString("EIP: ");
		Akari->Console->PutInt(r->callback.eip, 16);
		Akari->Console->PutString(", ESP: ");
		Akari->Console->PutInt(r->callback.esp, 16);
		Akari->Console->PutString(", EBP: ");
		Akari->Console->PutInt(r->callback.ebp, 16);
		Akari->Console->PutString(", CS: ");
		Akari->Console->PutInt(r->callback.cs, 16);
		Akari->Console->PutString(", EFLAGS: ");
		Akari->Console->PutInt(r->callback.eflags, 16);
		Akari->Console->PutString(", user ESP (may be garbage): ");
		Akari->Console->PutInt(r->useresp, 16);
		Akari->Console->PutString("\n");

		while (1)
			__asm__ __volatile__("hlt");
	}

	return resume;
}

void *irq_handler(struct modeswitch_registers *r) {
	ASSERT(r->callback.int_no >= 0x20);

	if (r->callback.int_no >= 0x28)			// IRQ 9+
		AkariOutB(0xA0, 0x20);		// EOI to slave IRQ controller
	AkariOutB(0x20, 0x20);			// EOI to master IRQ controller

	return Akari->Descriptor->irqt->CallHandler(r->callback.int_no - 0x20, r);
}

