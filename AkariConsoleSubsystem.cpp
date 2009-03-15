#include <AkariConsoleSubsystem.hpp>
#include <debug.hpp>

#define SCREEN_VMEM	0xb8000
#define PORT_CURSOR_IDX		0x3D4
#define PORT_CURSOR_DATA	0x3D4
#define PORT_CURSOR_MSB_IDX	0xE
#define PORT_CURSOR_LSB_IDX	0xF

#define SCREEN_BLANK	0x0720

AkariConsoleSubsystem::AkariConsoleSubsystem() {
	Clear();
}

u8 AkariConsoleSubsystem::VersionMajor() const { return 0; }
u8 AkariConsoleSubsystem::VersionMinor() const { return 1; }
const char *AkariConsoleSubsystem::VersionManufacturer() const { return "Akari"; }
const char *AkariConsoleSubsystem::VersionProduct() const { return "Akari Console"; }

void AkariConsoleSubsystem::Clear() {
	u16 *i = (u16 *)SCREEN_VMEM;

	for (u16 j = 0; j < 80 * 25; ++j, ++i)
		*i = SCREEN_BLANK;
	
	_CursorX = _CursorY = 0;
	_UpdateCursor();
}

void AkariConsoleSubsystem::PutChar(s8 c) {
	switch (c) {
		case '\n':	_ShiftCursorNewline(); break;
		case '\t':	_ShiftCursorTab(); break;
		default:
			*((s8 *)(SCREEN_VMEM + (_CursorY * 80 + _CursorX) * 2)) = c;
			_ShiftCursor();
	}
}

void AkariConsoleSubsystem::PutString(const char *s) {
	while (*s)
		PutChar(*s++);
}

void AkariConsoleSubsystem::PutStringN(const char *s, u32 n) {
	while (*s && n)
		PutChar(*s++), --n;
}

void AkariConsoleSubsystem::PutInt(u32 n, u8 base) {
	ASSERT(base >= 2 && base <= 36);
	
	u32 index = base;

	// TODO include logic so we don't go 'one over' and need to divide later
	while (index <= n)
		index *= base;

	do {
		index /= base;
		u8 c = (n / index);
		n -= (u32)c * index;

		PutChar( (c >= 0 && c <= 9) ? (c + '0') : (c - 10 + 'a') );
	} while (index > base);
}

void AkariConsoleSubsystem::_Scroll() {
	u16 i;
	while (_CursorY > 24) {
		for (i = 0; i < 24 * 80; ++i)
			((u16 *)SCREEN_VMEM)[i] = ((u16 *)SCREEN_VMEM)[i + 80];
		for (i = 0; i < 80; ++i)
			((u16 *)(SCREEN_VMEM + 24 * 80 * 2))[i] = SCREEN_BLANK;
		--_CursorY;
	}
}

void AkariConsoleSubsystem::_ShiftCursor() {
	if (++_CursorX >= 80) {
		_CursorX = 0;
		++_CursorY;
		_Scroll();
	} else
		_UpdateCursor();
}

void AkariConsoleSubsystem::_ShiftCursorTab() {
	if ((_CursorX = _CursorX - (_CursorX % 8) + 8) >= 80) {
		_CursorX = 0;
		++_CursorY;
		_Scroll();
	} else
		_UpdateCursor();
}

void AkariConsoleSubsystem::_ShiftCursorNewline() {
	_CursorX = 0;
	++_CursorY;
	_Scroll();
	_UpdateCursor();
}

void AkariConsoleSubsystem::_UpdateCursor() const {
	u16 index = _CursorX + 80 * _CursorY;
	AkariOutB(PORT_CURSOR_IDX, PORT_CURSOR_MSB_IDX);
	AkariOutB(PORT_CURSOR_DATA, (index >> 8) & 0xFF);
	AkariOutB(PORT_CURSOR_IDX, PORT_CURSOR_LSB_IDX);
	AkariOutB(PORT_CURSOR_DATA, index & 0xFF);
}

