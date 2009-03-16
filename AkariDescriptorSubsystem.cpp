#include <AkariDescriptorSubsystem.hpp>
#include <debug.hpp>

AkariDescriptorSubsystem::AkariDescriptorSubsystem(): _gdt(6) {
	_gdt.SetGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);	// code
	_gdt.SetGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);	// data
	_gdt.SetGate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);	// user code
	_gdt.SetGate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);	// user data
	// TODO write TSS into gdt
	
	_gdt.Flush();
	// TODO TSS flush
}

u8 AkariDescriptorSubsystem::VersionMajor() const { return 0; }
u8 AkariDescriptorSubsystem::VersionMinor() const { return 1; }
const char *AkariDescriptorSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariDescriptorSubsystem::VersionProduct() const { return "Akari Descriptor Table Manager"; }

AkariDescriptorSubsystem::GDT::GDT(u32 n): _entryCount(n) {
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

void AkariDescriptorSubsystem::GDT::Flush() {
	asm volatile("	\
		movl %0, %%eax; \
		lgdt (%%eax); \
		mov $0x10, %%eax; \
		movw %%ax, %%ds; \
		movw %%ax, %%es; \
		movw %%ax, %%fs; \
		movw %%ax, %%gs; \
		movw %%ax, %%ss; \
		ljmp $0x08, $.flush; \
	.flush:" : : "r" ((u32)&_pointer));
}

