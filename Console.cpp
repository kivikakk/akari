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

#include <Console.hpp>
#include <debug.hpp>
#include <stdarg.hpp>
#include <string>

#define SCREEN_VMEM	0xb8000
#define PORT_CURSOR_IDX		0x3D4
#define PORT_CURSOR_DATA	0x3D5
#define PORT_CURSOR_MSB_IDX	0xE
#define PORT_CURSOR_LSB_IDX	0xF

#define SCREEN_BLANK	0x0720

Console::Console() {
	clear();
}

u8 Console::versionMajor() const { return 0; }
u8 Console::versionMinor() const { return 1; }
const char *Console::versionManufacturer() const { return "Akari"; }
const char *Console::versionProduct() const { return "Akari Console"; }

void Console::clear() {
	u16 *i = reinterpret_cast<u16 *>(SCREEN_VMEM);

	for (u16 j = 0; j < 80 * 25; ++j, ++i)
		*i = SCREEN_BLANK;
	
	_cursorX = _cursorY = 0;
	_updateCursor();
}

void Console::putChar(s8 c) {
	switch (c) {
		case '\n':	_shiftCursorNewline(); break;
		case '\t':	_shiftCursorTab(); break;
		default:
			*reinterpret_cast<s8 *>(SCREEN_VMEM + (_cursorY * 80 + _cursorX) * 2) = c;
			_shiftCursor();
	}
}

void Console::putString(const char *s) {
	while (*s)
		putChar(*s++);
}

void Console::putStringN(const char *s, u32 n) {
	while (*s && n)
		putChar(*s++), --n;
}

void Console::printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);

	char *ret;
	vasprintf(&ret, format, ap);
	putString(ret);
	delete [] ret;

	va_end(ap);
}

void Console::_scroll() {
	u16 i;
	while (_cursorY > 24) {
		for (i = 0; i < 24 * 80; ++i)
			reinterpret_cast<u16 *>(SCREEN_VMEM)[i] = reinterpret_cast<u16 *>(SCREEN_VMEM)[i + 80];
		for (i = 0; i < 80; ++i)
			reinterpret_cast<u16 *>(SCREEN_VMEM + 24 * 80 * 2)[i] = SCREEN_BLANK;
		--_cursorY;
	}
}

void Console::_shiftCursor() {
	if (++_cursorX >= 80) {
		_cursorX = 0;
		++_cursorY;
		_scroll();
	} else
		_updateCursor();
}

void Console::_shiftCursorTab() {
	if ((_cursorX = _cursorX - (_cursorX % 8) + 8) >= 80) {
		_cursorX = 0;
		++_cursorY;
		_scroll();
	} else
		_updateCursor();
}

void Console::_shiftCursorNewline() {
	_cursorX = 0;
	++_cursorY;
	_scroll();
	_updateCursor();
}

void Console::_updateCursor() const {
	u16 index = _cursorX + 80 * _cursorY;
	AkariOutB(PORT_CURSOR_IDX, PORT_CURSOR_MSB_IDX);
	AkariOutB(PORT_CURSOR_DATA, (index >> 8) & 0xFF);
	AkariOutB(PORT_CURSOR_IDX, PORT_CURSOR_LSB_IDX);
	AkariOutB(PORT_CURSOR_DATA, index & 0xFF);
}
