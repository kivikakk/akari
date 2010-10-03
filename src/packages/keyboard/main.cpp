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

#include <stdio.hpp>
#include <UserCalls.hpp>
#include <arch.hpp>
#include <UserIPC.hpp>

static s8 keyboard_us[] = {
	0, 27, '1', '2', '3', '4', '5', '6', '7', '8',  // 0~9
	'9', '0', '-', '-', '\b',           			// ~14
	'\t', 'q', 'w', 'e', 'r',           			// ~19
	't', 'y', 'u', 'i', 'o', 'p',       			// ~25
	'[', ']', '\n', 0,              				// ~29 (CTRL)
	'a', 's', 'd', 'f', 'g', 'h',       			// ~35
	'j', 'k', 'l', ';', '\'',           			// ~40
	'`', 0, '\\', 'z', 'x', 'c',        			// ~46 (LSHFT)
	'v', 'b', 'n', 'm', ',',            			// ~51
	'.', '/', 0, '*',               				// ~55 (RSHFT)
	0, ' ', 0, 0, 0, 0, 0, 0, 0,        			// ~64 (ALT, CAPSL, F1~6)
	0, 0, 0, 0,                 					// ~68 (F7~10)
	0, 0, 0, 0, 0, '-',             				// ~74 (NUML, SCROLLL, HOME, UP, PGUP)
	0, 0, 0, '+',                   				// ~78 (LEFT, ?, RIGHT)
	0, 0, 0, 0, 0,                  				// ~83 (END, DOWN, PGDN, INS, DEL)
	0, 0, 0, 0, 0,                  				// ~88 (?, ?, ?, F11~12)
	0                       						// undefined
};

// Table of characters and their shifted equivalents.
static s8 keyboard_us_shift_table[] = {
	'`', '~',   '1', '!',   '2', '@',   '3', '#',
	'4', '$',   '5', '%',   '6', '^',   '7', '&',
	'8', '*',   '9', '(',   '0', ')',   '-', '_',
	'=', '+',   '[', '{',   ']', '}',   '\\', '|',
	';', ':',   '\'', '"',  ',', '<',   '.', '>',
	'/', '?',
	0
};

static void waitForKb() {
    while (1)
        if ((AkariInB(0x64) & 2) == 0)
            break;
}

static s8 shiftCharacter(s8 c) {
    if (c >= 'a' && c <= 'z')
        return c - 0x20;
    s8 *p = keyboard_us_shift_table;
    while (*p) {
        if (p[0] == c)
            return p[1];
        p += 2;
    }
    return c;
}

static s8 capslockInvert(s8 c) {
    if (c >= 'a' && c <= 'z')
        return c - 0x20;
    if (c >= 'A' && c <= 'Z')
        return c + 0x20;
    return c;
}


extern "C" int main() {
	// 128-bit=16 bytes bitfield
	u8 held_scancodes[16];     

	bool echo_mode = false;
	bool capslock_down = false, numlock_down = false, scrolllock_down = false;
	bool pressed_ctrl = false, pressed_alt = false, pressed_shift = false;

	if (!registerStream("input"))
		panic("could not register input");

	u32 myPid = processId();
	u32 writer = obtainStreamWriter(myPid, "input", true);
	if (writer == static_cast<u32>(-1)) {
		// This shouldn't be possible if we just initialised the damn thing.
		panic("could not obtain writer on own input");
	}

	irqListen(1);

	u8 scancode = AkariInB(0x60);
	bool mustUpdateLEDs = false;

	while (true) {
		if (scancode & 0x80) {
			// release
			scancode &= ~0x80;
			held_scancodes[scancode >> 3] &= ~(1 << (scancode & 7));
			if (scancode == 29)
				pressed_ctrl = false;
			else if (scancode == 42 || scancode == 54)
				pressed_shift = false;
			else if (scancode == 56)
				pressed_alt = false;
		} else {
			held_scancodes[scancode >> 3] |= 1 << (scancode & 7);
			if (scancode == 29)
				pressed_ctrl = true;
			else if (scancode == 42 || scancode == 54)
				pressed_shift = true;
			else if (scancode == 56)
				pressed_alt = true;
			else if (scancode == 58) {
				capslock_down = !capslock_down;
				mustUpdateLEDs = true;
			} else if (scancode == 69) {
				numlock_down = !numlock_down;
				mustUpdateLEDs = true;
			} else if (scancode == 70) {
				scrolllock_down = !scrolllock_down;
				mustUpdateLEDs = true;
			} else {
				scancode = keyboard_us[scancode];

				if (pressed_shift)
					scancode = shiftCharacter(scancode);

				if (capslock_down)
					scancode = capslockInvert(scancode);

				if (echo_mode)
					putc(scancode);

				// Now actually dispatch this.. reinterpret_cast for u8*<->char*. Unfortunate but true.
				writeStream(myPid, "input", writer, reinterpret_cast<const char *>(&scancode), 1);
			}

			if (mustUpdateLEDs) {
				u8 valuebyte =
					(scrolllock_down ? 1 : 0) |
					(numlock_down ? 2 : 0) |
					(capslock_down ? 4 : 0);

				waitForKb(); AkariOutB(0x60, 0xed);
				waitForKb(); AkariOutB(0x60, valuebyte);
			}
		}

		irqWait();
		scancode = AkariInB(0x60);
	}
}

