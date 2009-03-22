#include <AkariTaskSubsystem.hpp>
#include <Akari.hpp>

AkariTaskSubsystem::AkariTaskSubsystem() {
	u32 a;
	Akari->Console->PutString("stack variable is at 0x");
	Akari->Console->PutInt((u32)&a, 16);
	Akari->Console->PutChar('\n');
	Akari->Console->PutString("Akari is at 0x");
	Akari->Console->PutInt((u32)Akari, 16);
	Akari->Console->PutChar('\n');
	Akari->Console->PutString("&Akari is at 0x");
	Akari->Console->PutInt((u32)&Akari, 16);
	Akari->Console->PutChar('\n');
}

u8 AkariTaskSubsystem::VersionMajor() const { return 0; }
u8 AkariTaskSubsystem::VersionMinor() const { return 1; }
const char *AkariTaskSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariTaskSubsystem::VersionProduct() const { return "Akari Task Manager"; }

