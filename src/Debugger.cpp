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

static bool isReceivedFull() { return AkariInB(DEBUGGER_PORT + 5) & 0x01; }
static char receiveChar() {
	while (!isReceivedFull());
	return (char)AkariInB(DEBUGGER_PORT);
}
static u32 receiveU32() {
	u32 r = 0;
	for (int i = 0; i < 4; ++i)
		r |= (u32)((u8)receiveChar()) << i * 8;
	return r;
}
static std::string receiveNewline() {
	std::string s;
	while (true) {
		char c = receiveChar();
		if (c == '\n')
			return s;
		s += c;
	}
}

static bool isTransmitEmpty() { return AkariInB(DEBUGGER_PORT + 5) & 0x20; }
static void transmitChar(char c) {
	while (!isTransmitEmpty());
	AkariOutB(DEBUGGER_PORT, c);
}
static void transmit(u32 n) {
	for (int i = 0; i < 4; ++i) {
		transmitChar((char)((n >> i * 8) & 0xFF));
	}
}
static void transmit(const char *s) {
	while (*s)
		transmitChar(*s++);
}
static void transmit(const std::string &s) {
	transmit(s.c_str());
}

static bool initCOM(int com) {
	int port;

	switch (com) {
	case 1: port = 0x3f8; break;
	case 2: port = 0x2f8; break;
	case 3: port = 0x3e8; break;
	case 4: port = 0x2e8; break;
	default: return false;

	AkariOutB(port + 1, 0x00);
	AkariOutB(port + 3, 0x80);
	AkariOutB(port + 0, 0x03);	// 38,400
	AkariOutB(port + 1, 0x00);
	AkariOutB(port + 3, 0x03);	// 8 bits, no parity, one stop bit
	AkariOutB(port + 2, 0xC7);	// FIFO, clear, 14-byte threshold
	AkariOutB(port + 4, 0x0B);	// IRQs enabled, RTS/DSR set

	return true;
}

void Debugger::run() {
	initCOM(1);

	// TODO
}

