// This file is part of Akari.
// Copyright 2010 Arlen Cuss
//
// Akari is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Akari is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Akari.  If not, see <http://www.gnu.org/licenses/>.

#include <Akari.hpp>
#include <interrupts.hpp>
#include <Console.hpp>
#include <Descriptor.hpp>
#include <debug.hpp>
#include <Tasks.hpp>

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

		Akari->console->printf("EIP: %x, ESP: %x, EBP: %x, CS: %x, EFLAGS: %x, useresp: %x\nProcess 0x%x \"%s\" killed.\n",
			r->callback.eip, r->callback.esp, r->callback.ebp, r->callback.cs, r->callback.eflags, r->useresp,
			Akari->tasks->current->id, Akari->tasks->current->name.c_str());

		// TODO OMG: refactor this code! It's terrible! Half-copied from
		// UserCalls.cpp and the surrounding architecture to skip a killed
		// task in Syscalls. These can be unified, now.

		Tasks::Task *nextTask = Akari->tasks->prepareFetchNextTask();

		Tasks::Task **scanner = &Akari->tasks->start;
		while (*scanner != Akari->tasks->current) {
			scanner = &(*scanner)->next;
		}
		*scanner = (*scanner)->next;

		Akari->tasks->current = nextTask;
		return Akari->tasks->assignInternalTask(Akari->tasks->current);
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

