#include <AkariTimerSubsystem.hpp>
#include <arch.hpp>

AkariTimerSubsystem::AkariTimerSubsystem()
{ }

u8 AkariTimerSubsystem::versionMajor() const { return 0; }
u8 AkariTimerSubsystem::versionMinor() const { return 1; }
const char *AkariTimerSubsystem::versionManufacturer() const { return "Akari"; }
const char *AkariTimerSubsystem::versionProduct() const { return "Akari Timer Manager"; }

void AkariTimerSubsystem::setTimer(u16 hz) {
	u16 r = 0x1234dc / hz;
	AkariOutB(0x43, 0x36);		// 0b00110110; not BCD, square, LSB+MSB, c0
	AkariOutB(0x40, r & 0xFF);
	AkariOutB(0x40, (r >> 8) & 0xFF);
}

