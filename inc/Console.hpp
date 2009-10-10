#ifndef __CONSOLE_HPP__
#define __CONSOLE_HPP__

#include <Subsystem.hpp>

// Direct implementation.
class Console : public Subsystem {
	public:
		Console();

		u8 versionMajor() const;
		u8 versionMinor() const;
		const char *versionManufacturer() const;
		const char *versionProduct() const;

		void clear();
		void putChar(s8);
		void putString(const char *);
		void putStringN(const char *, u32);
		void putInt(u32, u8);

	protected:
		u8 _cursorX, _cursorY;

		void _scroll();

		void _shiftCursor();
		void _shiftCursorTab();
		void _shiftCursorNewline();

		void _updateCursor() const;
};

#endif

