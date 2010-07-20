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

#include <Debugger.hpp>
#include <arch.hpp>
#include <Akari.hpp>
#include <Console.hpp>

Debugger::Debugger()
{ }

u8 Debugger::versionMajor() const { return 0; }
u8 Debugger::versionMinor() const { return 1; }
const char *Debugger::versionManufacturer() const { return "Akari"; }
const char *Debugger::versionProduct() const { return "Akari Debugger"; }

void Debugger::run() {
	COMPort com1 = COMPort::COM(1);
	com1.initialize();

	while (true) {
		std::string command = com1.getMessage();

		if (command == "version") {
			com1.putU32(0);
			com1.putMessage(__AKARI_VERSION);
		} else if (command == "get") {
			u32 base = com1.getU32(),
				length = com1.getU32();

			com1.putU32(0);
			com1.putMemory(reinterpret_cast<char *>(base), length);
		} else if (command == "break") {
			com1.putU32(0);
			com1.putMessage("Bye!");
			break;
		} else {
			com1.putU32(1);
			com1.putMessage("unknown command");
		}
	}
}

COMPort::COMPort(u32 base): _base(base) { }

COMPort COMPort::COM(int com) {
	u32 port;

	switch (com) {
	case 1: port = 0x3f8; break;
	case 2: port = 0x2f8; break;
	case 3: port = 0x3e8; break;
	case 4: port = 0x2e8; break;
	default: port = 0x3f8; break;
	}

	return COMPort(port);
}

void COMPort::initialize() const {
	AkariOutB(_base + 1, 0x00);
	AkariOutB(_base + 3, 0x80);
	AkariOutB(_base + 0, 0x01);	// 115,200 baud
	AkariOutB(_base + 1, 0x00);
	AkariOutB(_base + 3, 0x03);	// 8 bits, no parity, one stop bit
	AkariOutB(_base + 2, 0xC7);	// FIFO, clear, 14-byte threshold
	AkariOutB(_base + 4, 0x0B);	// IRQs enabled, RTS/DSR set
}

char COMPort::getChar() const {
	while (!isReceivedFull());
	return static_cast<char>(AkariInB(_base));
}

u32 COMPort::getU32() const {
	u32 r = 0;
	for (int i = 0; i < 4; ++i)
		r |= static_cast<u32>(static_cast<u8>(getChar())) << i * 8;
	return r;
}
std::string COMPort::getStringNL() const {
	std::string s;
	while (true) {
		char c = getChar();
		if (c == '\n')
			return s;
		s += c;
	}
}
std::string COMPort::getString(u32 length) const {
	char *buf = new char[length];
	for (u32 i = 0; i < length; ++i)
		buf[i] = getChar();
	std::string s = std::string(buf, length);
	delete [] buf;
	return s;
}
std::string COMPort::getMessage() const {
	std::string s;
	u32 length = getU32();
	return getString(length);
}

void COMPort::putChar(char c) const {
	while (!isTransmitEmpty());
	AkariOutB(_base, c);
}
void COMPort::putU32(u32 n) const {
	for (int i = 0; i < 4; ++i) {
		putChar(static_cast<char>((n >> i * 8) & 0xFF));
	}
}
void COMPort::putMemory(const char *s, u32 n) const {
	while (n--)
		putChar(*s++);
}
void COMPort::putString(const char *s) const {
	while (*s)
		putChar(*s++);
}
void COMPort::putString(const std::string &s) const {
	putString(s.c_str());
}
void COMPort::putMessage(const std::string &s) const {
	putU32(s.length());
	putString(s);
}

bool COMPort::isReceivedFull() const {
	return AkariInB(_base + 5) & 0x01;
}

bool COMPort::isTransmitEmpty() const {
	return AkariInB(_base + 5) & 0x20;
}


