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

#include <Timer.hpp>
#include <arch.hpp>

u32 AkariMicrokernelSwitches = 0;

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

void Timer::tick() {
	++AkariMicrokernelSwitches;
}

