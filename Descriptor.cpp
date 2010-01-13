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

#include <Descriptor.hpp>
#include <Tasks.hpp>
#include <Akari.hpp>
#include <interrupts.hpp>
#include <debug.hpp>

Descriptor::Descriptor(): gdt(0), idt(0), irqt(0) {
	gdt = new GDT(10);
	gdt->setGate(1, 0, 0xFFFFFFFF, 0, true);	// code (CS)
	gdt->setGate(2, 0, 0xFFFFFFFF, 0, false);	// data (SS)
	gdt->setGate(3, 0, 0xFFFFFFFF, 1, true);	// driver code (CS)
	gdt->setGate(4, 0, 0xFFFFFFFF, 1, false);	// driver data (SS)
	gdt->clearGate(5);	// unused (code)
	gdt->clearGate(6);	// unused (data)
	gdt->setGate(7, 0, 0xFFFFFFFF, 3, true);	// user code (CS)
	gdt->setGate(8, 0, 0xFFFFFFFF, 3, false);	// user data (SS)
	gdt->writeTSS(9, 0x10, 0x0);		// empty ESP in TSS for now.. changed later in execution
	gdt->flush();
	gdt->flushTSS(9);

	idt = new IDT();

	irqt = new IRQT(idt);
}

u8 Descriptor::versionMajor() const { return 0; }
u8 Descriptor::versionMinor() const { return 1; }
const char *Descriptor::versionManufacturer() const { return "Akari"; }
const char *Descriptor::versionProduct() const { return "Akari Descriptor Table Manager"; }

Descriptor::GDT::GDT(u32 n): _entryCount(n), _entries(0) {
	_entries = new Entry[n];
	_pointer.limit = (sizeof(Entry) * n - 1);
	_pointer.base = (u32)_entries;

	setGate(0, 0, 0, 0, 0);
}

void Descriptor::GDT::setGateFields(s32 num, u32 base, u32 limit, u8 access, u8 granularity) {
	ASSERT(num >= 0 && num < static_cast<s32>(_entryCount));

	_entries[num].base_low		= (base & 0xFFFF);
	_entries[num].base_middle	= ((base >> 16) & 0xFF);
	_entries[num].base_high		= ((base >> 24) & 0xFF);

	_entries[num].limit_low		= (limit & 0xFFFF);
	_entries[num].granularity 	= (limit >> 16) & 0x0F;
	_entries[num].granularity 	|= granularity & 0xF0;
	_entries[num].access		= access;
}

void Descriptor::GDT::setGate(s32 num, u32 base, u32 limit, u8 dpl, bool code) {
	// access flag is like 0b1xx1yyyy, where yyyy = code?0xA:0x2 (just 'cause), and xx=DPL.
	setGateFields(num, base, limit, (code ? 0xA : 0x2) | (dpl << 5) | (0x9 << 4), 0xCF);
}

void Descriptor::GDT::clearGate(s32 num) {
	setGateFields(num, 0, 0, 0, 0);
}

void Descriptor::GDT::writeTSS(s32 num, u16 ss0, u32 esp0) {
	u32 base = reinterpret_cast<u32>(&_tssEntry);
	u32 limit = base + sizeof(TSSEntry);

	POSIX::memset(&_tssEntry, 0, sizeof(TSSEntry));
	
	_tssEntry.ss0 = ss0; _tssEntry.esp0 = esp0;
	_tssEntry.cs = 0x0b;	// 0x08 and 0x10 (kern code/data) + 0x3 (RPL ring 3)
	// NOTE: assumption about RPL is made right here!
	_tssEntry.ss = 0x13; _tssEntry.ds = 0x13;
	_tssEntry.es = 0x13; _tssEntry.fs = 0x13;
	_tssEntry.gs = 0x13;
	_tssEntry.iomap_base = reinterpret_cast<u32>(&_tssEntry.iomap) - reinterpret_cast<u32>(&_tssEntry);

	for (u16 i = 0; i < sizeof(_tssEntry.iomap); ++i)
		_tssEntry.iomap[i] = 0xFF;

	setGateFields(num, base, limit, 0xE9, 0x00);
}

void Descriptor::GDT::flush() {
	__asm__ __volatile__("	\
		movl %0, %%eax; \
		lgdt (%%eax); \
		mov $0x10, %%eax; \
		movw %%ax, %%ds; \
		movw %%ax, %%es; \
		movw %%ax, %%fs; \
		movw %%ax, %%gs; \
		movw %%ax, %%ss; \
		ljmp $0x08, $.flush; \
	.flush:" : : "r" (reinterpret_cast<u32>(&_pointer)) : "eax");
}

void Descriptor::GDT::flushTSS(s32 num) {
	__asm__ __volatile__("ltr %%ax" : : "a" ((num * 8) + 0x3));
}

void Descriptor::GDT::setTSSStack(u32 addr) {
	_tssEntry.esp0 = addr;
}

void Descriptor::GDT::setTSSIOMap(u8 *const &iomap) {
	POSIX::memcpy(_tssEntry.iomap, iomap, 8192);
}

Descriptor::IDT::IDT() {
	for (u16 i = 0; i < 0x100; ++i)
		_routines[i] = 0;

	for (u16 i = 0; i < 0x100; ++i)
		_entries[i].ulong = 0;

#define SET_IDT_GATE(n)	setGate(0x##n, isr##n, 0x08, 0x8e)
	SET_IDT_GATE(0); SET_IDT_GATE(1); SET_IDT_GATE(2); SET_IDT_GATE(3);
	SET_IDT_GATE(4); SET_IDT_GATE(5); SET_IDT_GATE(6); SET_IDT_GATE(7);
	SET_IDT_GATE(8); SET_IDT_GATE(9); SET_IDT_GATE(a); SET_IDT_GATE(b);
	SET_IDT_GATE(c); SET_IDT_GATE(d); SET_IDT_GATE(e); SET_IDT_GATE(f);
	SET_IDT_GATE(10); SET_IDT_GATE(11); SET_IDT_GATE(12); SET_IDT_GATE(13);
	SET_IDT_GATE(14); SET_IDT_GATE(15); SET_IDT_GATE(16); SET_IDT_GATE(17);
	SET_IDT_GATE(18); SET_IDT_GATE(19); SET_IDT_GATE(1a); SET_IDT_GATE(1b);
	SET_IDT_GATE(1c); SET_IDT_GATE(1d); SET_IDT_GATE(1e); SET_IDT_GATE(1f);
	SET_IDT_GATE(80);
#undef SET_IDT_GATE

	_pointer.limit = sizeof(_entries) - 1;
	_pointer.ridt = reinterpret_cast<u32>(_entries);

	__asm__ __volatile__("lidt %0" : : "m" (_pointer));
}

void Descriptor::IDT::installHandler(u8 isr, isr_handler_func_t callback) {
	ASSERT(isr >= 0 && isr <= 0xFF);
	_routines[isr] = callback;
}

void Descriptor::IDT::clearHandler(u8 isr) {
	installHandler(isr, 0);
}

void *Descriptor::IDT::callHandler(u8 isr, struct modeswitch_registers *regs) {
	if (_routines[isr])
		return _routines[isr](regs);
	return 0;
}

void Descriptor::IDT::setGate(u8 idt, void (*callback)(), u16 isrSegment, u8 flags) {
	_entries[idt].offset_low = reinterpret_cast<u32>(callback) & 0xFFFF;
	_entries[idt].selector = isrSegment;
	_entries[idt]._always_0 = 0;
	_entries[idt].flags = flags | 0x60;
	_entries[idt].offset_high = (reinterpret_cast<u32>(callback) >> 16) & 0xFFFF;
}

Descriptor::IRQT::IRQT(IDT *idt): _idt(idt) {
	AkariOutB(0x20, 0x11);
	AkariOutB(0xA0, 0x11);
	AkariOutB(0x21, 0x20);
	AkariOutB(0xA1, 0x28);
	AkariOutB(0x21, 0x04);
	AkariOutB(0xA1, 0x02);
	AkariOutB(0x21, 0x01);
	AkariOutB(0xA1, 0x01);
	AkariOutB(0x21, 0x00);
	AkariOutB(0xA1, 0x00);

#define SET_IRQ_GATE(n) idt->setGate(0x2##n, irq##n, 0x08, 0x8e)
	SET_IRQ_GATE(0); SET_IRQ_GATE(1); SET_IRQ_GATE(2); SET_IRQ_GATE(3);
	SET_IRQ_GATE(4); SET_IRQ_GATE(5); SET_IRQ_GATE(6); SET_IRQ_GATE(7);
	SET_IRQ_GATE(8); SET_IRQ_GATE(9); SET_IRQ_GATE(a); SET_IRQ_GATE(b);
	SET_IRQ_GATE(c); SET_IRQ_GATE(d); SET_IRQ_GATE(e); SET_IRQ_GATE(f);
#undef SET_IRQ_GATE

	for (u8 i = 0; i < 16; ++i)
		_routines[i] = 0;
}

void Descriptor::IRQT::installHandler(u8 irq, irq_handler_func_t callback) {
	// you can't set IRQ 0 (timer) here - it's done manually. see interrupts.s
	ASSERT(irq >= 1 && irq <= 0x0f);
	_routines[irq] = callback;
}

void Descriptor::IRQT::clearHandler(u8 irq) {
	installHandler(irq, 0);
}

void *Descriptor::IRQT::callHandler(u8 irq, struct modeswitch_registers *regs) {
	Tasks::Task *iter = Akari->tasks->start;
	while (iter) {
		if (iter->irqListen == irq) {
			iter->irqListenHits++;

			// We're not using this out of a sort of an interest to see if we can deal with not
			// returning immediately. It looks like it works, but at the same time, I'd say there
			// are advantages to prioritising the I/O. Wait and see.

			// Looks like it's their turn. We append the current `iter` to the linked
			// list made of ATS#priorityStart and the Task's own priorityNext's.
			Tasks::Task **append = &Akari->tasks->priorityStart;
			while (*append)
				append = &(*append)->priorityNext;
			*append = iter;
			iter->priorityNext = 0;
		}
		iter = iter->next;
	}

	// If there's a priority, switch to it immediately. Crap. What do we do about handlers?!
	if (Akari->tasks->priorityStart) {
		iter = Akari->tasks->priorityStart;
		Akari->tasks->priorityStart = iter->priorityNext;
		iter->priorityNext = 0;

		// Task switch!
		Akari->tasks->saveRegisterToTask(Akari->tasks->current, regs);
		Akari->tasks->current = iter;
		return Akari->tasks->assignInternalTask(Akari->tasks->current);
	}
		

	// XXX XXX TODO HACK OMG FIXME BBQ (see above one-liner comment)
	// Possible conflict between an application wanting to listen for an IRQ, and a routine
	// being installed. Douuuuuuuushiyou
	if (_routines[irq]) {
		AkariPanic("lol");	// I'm assuming this isn't going to be called for now ...
		return _routines[irq](regs);
	}

	return regs;
}

