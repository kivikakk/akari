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

#ifndef __SYSCALL_HPP__
#define __SYSCALL_HPP__

#include <Tasks.hpp>

#define AKARI_SYSCALL_MAXCALLS 256

class Syscall {
	public:
		typedef u32(*syscall_fn_t)();

		Syscall();

		explicit Syscall(const Syscall &);
		Syscall &operator =(const Syscall &);

		void addSyscall(u16 num, syscall_fn_t fn);

		void returnToTask(Tasks::Task *task);
		void returnToNextTask();

	protected:
		static void *_handler(struct modeswitch_registers *);

		syscall_fn_t _syscalls[AKARI_SYSCALL_MAXCALLS];
		u16 _syscalls_assigned;

		Tasks::Task *_returnTask;
};

#endif

