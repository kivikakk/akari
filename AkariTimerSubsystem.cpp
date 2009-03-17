#include <AkariTimerSubsystem.hpp>
#include <arch.hpp>

AkariTimerSubsystem::AkariTimerSubsystem()
{ }

u8 AkariTimerSubsystem::VersionMajor() const { return 0; }
u8 AkariTimerSubsystem::VersionMinor() const { return 1; }
const char *AkariTimerSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariTimerSubsystem::VersionProduct() const { return "Akari Timer Manager"; }

void AkariTimerSubsystem::SetTimer(u16 hz) {
	u16 r = 0x1234dc / hz;
	AkariOutB(0x43, 0x36);		// 0b00110110; not BCD, square, LSB+MSB, c0
	AkariOutB(0x40, r & 0xFF);
	AkariOutB(0x40, (r >> 8) & 0xFF);
}

