#include <AkariDescriptorSubsystem.hpp>

AkariDescriptorSubsystem::AkariDescriptorSubsystem() {
}

u8 AkariDescriptorSubsystem::VersionMajor() const { return 0; }
u8 AkariDescriptorSubsystem::VersionMinor() const { return 1; }
const char *AkariDescriptorSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariDescriptorSubsystem::VersionProduct() const { return "Akari Descriptor Table Manager"; }

void AkariDescriptorSubsystem::GDT::SetGate(s32 num, u32 base, u32 limit, u8 access, u8 granularity) {
	_entries[num].base_low		= (base & 0xFFFF);
	_entries[num].base_middle	= ((base >> 16) & 0xFF);
	_entries[num].base_high		= ((base >> 24) & 0xFF);

	_entries[num].limit_low		= (limit & 0xFFFF);
	_entries[num].granularity 	= (limit >> 16) & 0x0F;
	_entries[num].granularity 	|= granularity & 0xF0;
	_entries[num].access		= access;
}

