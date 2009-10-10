#include <Akari.hpp>
#include <arch.hpp>
#include <UserGates.hpp>

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

static bool echo_mode = true;
static bool capslock_down = false, numlock_down = false, scrolllock_down = false;
static bool pressed_ctrl = false, pressed_alt = false, pressed_shift = false;
static u8 held_scancodes[16];     // held_scancodes is a 128-bit=16 bytes bitfield static

static void waitForKb() {
    while (1)
        if ((AkariInB(0x64) & 2) == 0)
            break;
}

static void updateLEDs() {
    u8 valuebyte =
        (scrolllock_down ? 1 : 0) |
        (numlock_down ? 2 : 0) |
        (capslock_down ? 4 : 0);

    waitForKb(); AkariOutB(0x60, 0xed);
    waitForKb(); AkariOutB(0x60, valuebyte);
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


void KeyboardProcess() {
	for (int i = 0; i < 1000000; ++i);

	if (!SYSCALL_BOOL(syscall_registerName("system.io.keyboard")))
		syscall_panic("could not register system.io.keyboard");

	if (!SYSCALL_BOOL(syscall_registerNode("input")))
		syscall_panic("could not register system.io.keyboard:input");

	u32 writer = syscall_obtainNodeWriter("system.io.keyboard", "input", true);
	if (writer == (u32)-1) {
		// This shouldn't be possible if we just initialised the damn thing.
		syscall_panic("could not obtain writer on system.io.keyboard:input");
	}

	syscall_puts("Kb all good.\n");

	syscall_irqListen(1);

	u8 scancode = AkariInB(0x60);
	while (1) {
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
				updateLEDs();
			} else if (scancode == 69) {
				numlock_down = !numlock_down;
				updateLEDs();
			} else if (scancode == 70) {
				scrolllock_down = !scrolllock_down;
				updateLEDs();
			} else {
				scancode = keyboard_us[scancode];
				if (pressed_shift)
					scancode = shiftCharacter(scancode);
				if (capslock_down)
					scancode = capslockInvert(scancode);
				if (echo_mode)
					syscall_putc(scancode);

				// Now actually dispatch this.
				syscall_writeNode("system.io.keyboard", "input", writer, (const char *)&scancode, 1);
			}
		}

		syscall_irqWait();
		scancode = AkariInB(0x60);
	}
}

