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

	void *resume = Akari->descriptor->idt->callHandler(r->callback.int_no, r);
	if (!resume) {
		// nothing to call!
		Akari->console->putChar('\n');
		Akari->console->putString((r->callback.int_no < 32 && isr_messages[r->callback.int_no]) ? isr_messages[r->callback.int_no] : "[Intel reserved]");
		Akari->console->putString(" exception occured!\n");

		Akari->console->putString("EIP: ");
		Akari->console->putInt(r->callback.eip, 16);
		Akari->console->putString(", ESP: ");
		Akari->console->putInt(r->callback.esp, 16);
		Akari->console->putString(", EBP: ");
		Akari->console->putInt(r->callback.ebp, 16);
		Akari->console->putString(", CS: ");
		Akari->console->putInt(r->callback.cs, 16);
		Akari->console->putString(", EFLAGS: ");
		Akari->console->putInt(r->callback.eflags, 16);
		Akari->console->putString(", user ESP (may be garbage): ");
		Akari->console->putInt(r->useresp, 16);
		Akari->console->putString("\n");

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

	return Akari->descriptor->irqt->callHandler(r->callback.int_no - 0x20, r);
}

