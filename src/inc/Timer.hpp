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

extern u32 AkariMicrokernelSwitches, AkariTickHz;

class TimerEvent {
public:
	TimerEvent(u32 at);
	virtual ~TimerEvent();

	virtual void operator()() = 0;

	u32 at;
};

class Timer {
public:
	Timer();

	void setTimer(u16);
	void tick();

	void at(counted_ptr<TimerEvent> event);
	void desched(const TimerEvent &event);

protected:
	u32 time_til_next;
	std::list< counted_ptr<TimerEvent> > events;
};

class TimerEventWakeup : public TimerEvent {
public:
	TimerEventWakeup(u32 at, Tasks::Task *task);

	explicit TimerEventWakeup(const TimerEventWakeup &);
	TimerEventWakeup &operator =(const TimerEventWakeup &);

	void operator()();

protected:
	Tasks::Task *wakeup;
};

#endif

