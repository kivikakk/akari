#ifndef __AKARI_CONSOLE_SUBSYSTEM_HPP__
#define __AKARI_CONSOLE_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>

// Direct implementation.
class AkariConsoleSubsystem : public AkariSubsystem {
	public:
		AkariConsoleSubsystem(Akari *);

		u16 VersionMajor() const;
		u16 VersionMinor() const;

		void Clear();
		void PutChar(s8);
		void PutString(const s8 *);
		void PutStringN(const s8 *, u32);
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

