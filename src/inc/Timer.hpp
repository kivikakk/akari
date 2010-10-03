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

#ifndef __TIMER_HPP__
#define __TIMER_HPP__

#include <Tasks.hpp>
#include <list>
#include <counted_ptr>

extern u32 AkariMicrokernelSwitches, AkariTickHz, AkariSystemUs;

class WakeEvent {
public:
	explicit WakeEvent();
	WakeEvent(const WakeEvent &);

	Tasks::Task *toWake;
	u32 uid;
	u32 usLeft;
};

class Timer {
public:
	Timer();

	void setTimer(u16);
	void tick();

	u32 wakeIn(u32 us, Tasks::Task *task);
	bool desched(u32 uid);

protected:
	std::list<WakeEvent> wakeevents;
};

#endif

