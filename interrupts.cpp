#include <Akari.hpp>
#include <interrupts.hpp>

void isr_handler(struct registers r) {
	Akari->Console->PutString("Got interrupt: ");
	Akari->Console->PutInt(r.int_no, 16);
	Akari->Console->PutChar('\n');
}

void irq_handler(struct registers r) {
	if (r.int_no >= 40)				// IRQ 9+
		AkariOutB(0xA0, 0x20);		// EOI to slave IRQ controller
	AkariOutB(0x20, 0x20);			// EOI to master IRQ controller

	Akari->Console->PutString("Got IRQ: ");
	Akari->Console->PutInt(r.int_no - 0x20, 16);
	Akari->Console->PutChar('\n');

	Akari->Descriptor->_irqt->CallHandler(r.int_no - 0x20, r);
}
