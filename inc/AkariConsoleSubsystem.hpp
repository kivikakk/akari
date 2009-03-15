#ifndef __AKARI_CONSOLE_SUBSYSTEM_HPP__
#define __AKARI_CONSOLE_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>

// Direct implementation.
class AkariConsoleSubsystem : public AkariSubsystem {
	public:
		AkariConsoleSubsystem(Akari *);

		u8 VersionMajor() const;
		u8 VersionMinor() const;
		const char *VersionManufacturer() const;
		const char *VersionProduct() const;

		void Clear();
		void PutChar(s8);
		void PutString(const char *);
		void PutStringN(const char *, u32);
		void PutInt(u32, u8);

	protected:
		u8 _CursorX, _CursorY;

		void _Scroll();

		void _ShiftCursor();
		void _ShiftCursorTab();
		void _ShiftCursorNewline();

		void _UpdateCursor() const;
};

#endif

