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

#ifndef __USER_IPC_HPP__
#define __USER_IPC_HPP__

#include <arch.hpp>
#include <UserGates.hpp>

#ifdef __AKARI_KERNEL__

#include <Tasks.hpp>

namespace User {
namespace IPC {
	pid_t processId();
	pid_t processIdByName(const char *name);
	bool registerName(const char *name);

	bool registerStream(const char *node);
	u32 obtainStreamWriter(pid_t id, const char *node, bool exclusive);
	u32 obtainStreamListener(pid_t id, const char *node);
	u32 readStream(pid_t id, const char *node, u32 listener, char *buffer, u32 n);
	u32 readStreamUnblock(pid_t id, const char *node, u32 listener, char *buffer, u32 n);
	u32 writeStream(pid_t id, const char *node, u32 writer, const char *buffer, u32 n);

	class ReadStreamCall : public BlockingCall {
	public:
		ReadStreamCall(pid_t id, const char *node, u32 listener, char *buffer, u32 n);

		Tasks::Task::Stream::Listener *getListener() const;
		u32 operator ()();

		static Symbol type();
		Symbol insttype() const;

	protected:
		Tasks::Task::Stream::Listener *_listener;
		char *_buffer;
		u32 _n;
	};
}
}

#else

DECL_SYSCALL0(processId, pid_t);
DECL_SYSCALL1(processIdByName, pid_t, const char *);
DECL_SYSCALL1(registerName, bool, const char *);

DECL_SYSCALL1(registerStream, bool, const char *);
DECL_SYSCALL3(obtainStreamWriter, u32, pid_t, const char *, bool);
DECL_SYSCALL2(obtainStreamListener, u32, pid_t, const char *);
DECL_SYSCALL5(readStream, u32, pid_t, const char *, u32, char *, u32);
DECL_SYSCALL5(readStreamUnblock, u32, pid_t, const char *, u32, char *, u32);
DECL_SYSCALL5(writeStream, u32, pid_t, const char *, u32, const char *, u32);

#endif

#endif

