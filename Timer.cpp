#include <Timer.hpp>
#include <arch.hpp>

Timer::Timer()
{ }

u8 Timer::versionMajor() const { return 0; }
u8 Timer::versionMinor() const { return 1; }
const char *Timer::versionManufacturer() const { return "Akari"; }
const char *Timer::versionProduct() const { return "Akari Timer Manager"; }

void Timer::setTimer(u16 hz) {
	u16 r = 0x1234dc / hz;
	AkariOutB(0x43, 0x36);		// 0b00110110; not BCD, square, LSB+MSB, c0
	AkariOutB(0x40, r & 0xFF);
	AkariOutB(0x40, (r >> 8) & 0xFF);
}

