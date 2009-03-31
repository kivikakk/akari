#include <AkariDescriptorSubsystem.hpp>
#include <Akari.hpp>
#include <interrupts.hpp>
#include <debug.hpp>

AkariDescriptorSubsystem::AkariDescriptorSubsystem(): _gdt(0), _idt(0), _irqt(0) {
	_gdt = new GDT(6);
	_gdt->SetGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);	// code (CS)
	_gdt->SetGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);	// data (SS)
	_gdt->SetGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);	// user code (CS)
	_gdt->SetGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);	// user data (SS)
	_gdt->WriteTSS(5, 0x10, 0x0);		// empty ESP in TSS for now.. changed later in execution
	_gdt->Flush();
	_gdt->FlushTSS(5);

	_idt = new IDT();

	_irqt = new IRQT(_idt);
}

u8 AkariDescriptorSubsystem::VersionMajor() const { return 0; }
u8 AkariDescriptorSubsystem::VersionMinor() const { return 1; }
const char *AkariDescriptorSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariDescriptorSubsystem::VersionProduct() const { return "Akari Descriptor Table Manager"; }

AkariDescriptorSubsystem::GDT::GDT(u32 n): _entryCount(n), _entries(0) {
	_entries = new Entry[n];
	_pointer.limit = (sizeof(Entry) * n - 1);
	_pointer.base = (u32)_entries;

	SetGate(0, 0, 0, 0, 0);
}

void AkariDescriptorSubsystem::GDT::SetGate(s32 num, u32 base, u32 limit, u8 access, u8 granularity) {
	ASSERT(num >= 0 && num < (s32)_entryCount);

	_entries[num].base_low		= (base & 0xFFFF);
	_entries[num].base_middle	= ((base >> 16) & 0xFF);
	_entries[num].base_high		= ((base >> 24) & 0xFF);

	_entries[num].limit_low		= (limit & 0xFFFF);
	_entries[num].granularity 	= (limit >> 16) & 0x0F;
	_entries[num].granularity 	|= granularity & 0xF0;
	_entries[num].access		= access;
}

void AkariDescriptorSubsystem::GDT::WriteTSS(s32 num, u16 ss0, u32 esp0) {
	u32 base = (u32)&_tssEntry;
	u32 limit = base + sizeof(TSSEntry);

	POSIX::memset(&_tssEntry, 0, sizeof(TSSEntry));
	
	_tssEntry.ss0 = ss0; _tssEntry.esp0 = esp0;
	_tssEntry.cs = 0x0b;	// 0x08 and 0x10 (kern code/data) + 0x3 (RPL ring 3)
	_tssEntry.ss = 0x13; _tssEntry.ds = 0x13;
	_tssEntry.es = 0x13; _tssEntry.fs = 0x13;
	_tssEntry.gs = 0x13;

	SetGate(num, base, limit, 0xE9, 0x00);
}

void AkariDescriptorSubsystem::GDT::Flush() {
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
	.flush:" : : "r" ((u32)&_pointer) : "eax");
}

void AkariDescriptorSubsystem::GDT::FlushTSS(s32 num) {
	__asm__ __volatile__("ltr %%ax" : : "a" ((num * 8) + 0x3));
}

void AkariDescriptorSubsystem::GDT::SetTSSStack(u32 addr) {
	_tssEntry.esp0 = addr;
}

AkariDescriptorSubsystem::IDT::IDT() {
	for (u16 i = 0; i < 0x100; ++i)
		_routines[i] = 0;

	for (u16 i = 0; i < 0x100; ++i)
		_entries[i].ulong = 0;

#define SET_IDT_GATE(n)	SetGate(0x##n, isr##n, 0x08, 0x8e)
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
	_pointer.ridt = (u32)_entries;

	__asm__ __volatile__("lidt %0" : : "m" (_pointer));
}

void AkariDescriptorSubsystem::IDT::InstallHandler(u8 isr, isr_handler_func_t callback) {
	ASSERT(isr >= 0 && isr <= 0xFF);
	_routines[isr] = callback;
}

void AkariDescriptorSubsystem::IDT::ClearHandler(u8 isr) {
	InstallHandler(isr, 0);
}

bool AkariDescriptorSubsystem::IDT::CallHandler(u8 isr, struct callback_registers regs) {
	if (_routines[isr]) {
		_routines[isr](regs);
		return true;
	}
	return false;
}

void AkariDescriptorSubsystem::IDT::SetGate(u8 idt, void (*callback)(), u16 isrSegment, u8 flags) {
	_entries[idt].offset_low = (u32)callback & 0xFFFF;
	_entries[idt].selector = isrSegment;
	_entries[idt]._always_0 = 0;
	_entries[idt].flags = flags | 0x60;
	_entries[idt].offset_high = ((u32)callback >> 16) & 0xFFFF;
}

AkariDescriptorSubsystem::IRQT::IRQT(IDT *idt): _idt(idt) {
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

#define SET_IRQ_GATE(n) _idt->SetGate(0x2##n, irq##n, 0x08, 0x8e)
	SET_IRQ_GATE(0); SET_IRQ_GATE(1); SET_IRQ_GATE(2); SET_IRQ_GATE(3);
	SET_IRQ_GATE(4); SET_IRQ_GATE(5); SET_IRQ_GATE(6); SET_IRQ_GATE(7);
	SET_IRQ_GATE(8); SET_IRQ_GATE(9); SET_IRQ_GATE(a); SET_IRQ_GATE(b);
	SET_IRQ_GATE(c); SET_IRQ_GATE(d); SET_IRQ_GATE(e); SET_IRQ_GATE(f);
#undef SET_IRQ_GATE

	for (u8 i = 0; i < 16; ++i)
		_routines[i] = 0;
}

void AkariDescriptorSubsystem::IRQT::InstallHandler(u8 irq, irq_handler_func_t callback) {
	ASSERT(irq >= 0 && irq <= 0x0f);
	_routines[irq] = callback;
}

void AkariDescriptorSubsystem::IRQT::ClearHandler(u8 irq) {
	InstallHandler(irq, 0);
}

void AkariDescriptorSubsystem::IRQT::CallHandler(u8 irq, struct callback_registers *regs) {
	if (_routines[irq])
		_routines[irq](regs);
}

