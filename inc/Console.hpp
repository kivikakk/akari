#ifndef __AKARI_CONSOLE_SUBSYSTEM_HPP__
#define __AKARI_CONSOLE_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>

// Direct implementation.
class AkariConsoleSubsystem : public AkariSubsystem {
	public:
		AkariConsoleSubsystem();

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

