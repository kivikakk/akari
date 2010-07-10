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

#include <debug.hpp>
#include <arch.hpp>
#include <Akari.hpp>
#include <Console.hpp>

#define SCREEN_VMEM		0xb8000
#define SCREEN_DEATH	0x4f

static const char *PANIC_MSG = "oh god how did this get here I am not good with kernel panic";

void AkariPanic(const char *message) {
	u16 *i = reinterpret_cast<u16 *>(SCREEN_VMEM);
	for (u16 j = 0; j < 80 * 25; ++j, ++i)
		*(reinterpret_cast<u8 *>(i) + 1) = SCREEN_DEATH;
	
	if (Akari && Akari->console) {
		Akari->console->putString(PANIC_MSG);
		Akari->console->putString("\n");
		Akari->console->putString(message);
	} else {
		// "Clever."
		i = reinterpret_cast<u16 *>(SCREEN_VMEM);
		while (*PANIC_MSG)
			*reinterpret_cast<s8 *>(i++) = *PANIC_MSG++;

		i = reinterpret_cast<u16 *>(SCREEN_VMEM) + 80;
		while (*message)
			*reinterpret_cast<s8 *>(i++) = *message++;
	}
	
	AkariHalt();
}

void AkariHalt() {
	asm volatile("cli");
	while (true)
		asm volatile("hlt");
}

