#include <Akari.hpp>
#include <interrupts.hpp>

void isr_handler(struct registers r) {
	Akari->Console->PutString("Got interrupt: ");
	Akari->Console->PutInt(r.int_no, 16);
	Akari->Console->PutChar('\n');
}

void irq_handler(struct registers r) {
	if (r.int_no >= 40)
		AkariOutB(0xa0, 0x20);		// EOI to slave IRQ controller
	AkariOutB(0x20, 0x20);			// EOI to master IRQ controller

	Akari->Console->PutString("Got IRQ: ");
	Akari->Console->PutInt(r.int_no, 16);
	Akari->Console->PutChar('\n');
}
