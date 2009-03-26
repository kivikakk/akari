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

void isr_handler(struct registers r) {
	ASSERT(r.int_no < 0x20 || r.int_no >= 0x30);
	// I guess this is right? What other IDTs are there? Hm.

	if (!Akari->Descriptor->_idt->CallHandler(r.int_no, r)) {
		// nothing to call!
		Akari->Console->PutChar('\n');
		Akari->Console->PutString((r.int_no < 32 && isr_messages[r.int_no]) ? isr_messages[r.int_no] : "[Intel reserved]");
		Akari->Console->PutString(" exception occured!\n");

		// TODO We don't know these yet.
		/*
		Akari->Console->PutString("EIP: ");
		Akari->Console->PutInt(eip);
		Akari->Console->PutString("ESP: ");
		Akari->Console->PutInt(esp);
		Akari->Console->PutString("EBP: ");
		Akari->Console->PutInt(ebp);
		*/

		while (1)
			__asm__ __volatile__("hlt");
	}
}

void irq_handler(struct registers *r) {
	if (r->int_no >= 0x28)			// IRQ 9+
		AkariOutB(0xA0, 0x20);		// EOI to slave IRQ controller
	AkariOutB(0x20, 0x20);			// EOI to master IRQ controller

	Akari->Descriptor->_irqt->CallHandler(r->int_no - 0x20, r);
}

