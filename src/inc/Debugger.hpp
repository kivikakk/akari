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

#ifndef __DEBUGGER_HPP__
#define __DEBUGGER_HPP__

#include <Subsystem.hpp>
#include <Tasks.hpp>
#include <string>

class Debugger : public Subsystem {
public:
	Debugger();

	u8 versionMajor() const;
	u8 versionMinor() const;
	const char *versionManufacturer() const;
	const char *versionProduct() const;

	void run();
};

class COMPort {
public:
	COMPort(u32 base);
	static COMPort COM(int com);

	void initialize() const;

	char getChar() const;
	u16 getU16() const;
	u32 getU32() const;
	std::string getStringNL() const;
	std::string getString(u32 length) const;
	std::string getMessage() const;

	void putChar(char c) const;
	void putU16(u16 n) const;
	void putU32(u32 n) const;
	void putMemory(const char *s, u32 n) const;
	void putString(const char *s) const;
	void putString(const std::string &s) const;
	void putMessage(const std::string &s) const;

protected:
	u32 _base;

	bool isReceivedFull() const;
	bool isTransmitEmpty() const;
};

#endif

