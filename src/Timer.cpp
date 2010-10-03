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

#include <Timer.hpp>
#include <arch.hpp>
#include <algorithm>
#include <Akari.hpp>
#include <Console.hpp>

u32 AkariMicrokernelSwitches = 0, AkariTickHz = 0, AkariSystemUs = 0;

static u32 lastWakeId = 0;		// Note that many use comparisons to 0 to
					// check that a given lastWakeId is invalid,
					// so make sure 0 is never handed out as one. :/
u32 usPerTick = 0;

WakeEvent::WakeEvent(): toWake(0), uid(++lastWakeId), usLeft(0)
{ }

WakeEvent::WakeEvent(const WakeEvent &r): toWake(r.toWake), uid(r.uid), usLeft(r.usLeft)
{ }

Timer::Timer(): wakeevents()
{ }

void Timer::setTimer(u16 hz) {
	AkariTickHz = hz;
	usPerTick = 1000000 / hz;
	if (1000000 % hz) {
		mu_console->putString("Timer: WARNING: selected timer frequency doesn't divide into a second\n");
	}

	u16 r = 0x1234dc / hz;
	AkariOutB(0x43, 0x36);		// 0b00110110; not BCD, square, LSB+MSB, c0
	AkariOutB(0x40, r & 0xFF);
	AkariOutB(0x40, (r >> 8) & 0xFF);
}

void Timer::tick() {
	++AkariMicrokernelSwitches;
	AkariSystemUs += usPerTick;
	
	if (wakeevents.size()) {
		std::list<WakeEvent>::iterator top = wakeevents.begin();
		if (top->usLeft > usPerTick)
			top->usLeft -= usPerTick;
		else {
			top->toWake->userWaiting = false;
			wakeevents.pop_front();
		}
	}
}

u32 Timer::wakeIn(u32 us, Tasks::Task *task) {
	WakeEvent event;
	event.toWake = task;

	if (!wakeevents.size()) {
		event.usLeft = us;
		wakeevents.push_back(event);
		return event.uid;
	}

	u32 runningTotal = 0;
	for (std::list<WakeEvent>::iterator it = wakeevents.begin(); it != wakeevents.end(); ++it) {
		if (us < runningTotal + it->usLeft) break;
		runningTotal += it->usLeft;
	}


	event.usLeft = us - runningTotal;
	wakeevents.push_back(event);
	return event.uid;
}

bool Timer::desched(u32 uid) {
	for (std::list<WakeEvent>::iterator it = wakeevents.begin(); it != wakeevents.end(); ++it) {
		if (it->uid == uid) {
			u32 wait = it->usLeft;
			std::list<WakeEvent>::iterator addIt = it;
			if (++addIt != wakeevents.end())
				addIt->usLeft += wait;
			wakeevents.erase(it);
			return true;
		}
	}
	return false;
}


