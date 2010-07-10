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

#if defined(__AKARI_KERNEL)

#include <Tasks.hpp>

namespace User {
namespace IPC {
	pid_t processId();
	pid_t processIdByName(const char *name);
	pid_t processIdByNameBlock(const char *name);
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

#elif defined(__AKARI_LINKAGE)

DEFN_SYSCALL0(processId, 24, pid_t);
DEFN_SYSCALL1(processIdByName, 25, pid_t, const char *);
DEFN_SYSCALL1(processIdByNameBlock, 39, pid_t, const char *);
DEFN_SYSCALL1(registerName, 12, bool, const char *);

DEFN_SYSCALL1(registerStream, 13, bool, const char *);
DEFN_SYSCALL3(obtainStreamWriter, 14, u32, pid_t, const char *, bool);
DEFN_SYSCALL2(obtainStreamListener, 15, u32, pid_t, const char *);
DEFN_SYSCALL5(readStream, 16, u32, pid_t, const char *, u32, char *, u32);
DEFN_SYSCALL5(readStreamUnblock, 17, u32, pid_t, const char *, u32, char *, u32);
DEFN_SYSCALL5(writeStream, 18, u32, pid_t, const char *, u32, const char *, u32);

#else

DECL_SYSCALL0(processId, pid_t);
DECL_SYSCALL1(processIdByName, pid_t, const char *);
DECL_SYSCALL1(processIdByNameBlock, pid_t, const char *);
DECL_SYSCALL1(registerName, bool, const char *);

DECL_SYSCALL1(registerStream, bool, const char *);
DECL_SYSCALL3(obtainStreamWriter, u32, pid_t, const char *, bool);
DECL_SYSCALL2(obtainStreamListener, u32, pid_t, const char *);
DECL_SYSCALL5(readStream, u32, pid_t, const char *, u32, char *, u32);
DECL_SYSCALL5(readStreamUnblock, u32, pid_t, const char *, u32, char *, u32);
DECL_SYSCALL5(writeStream, u32, pid_t, const char *, u32, const char *, u32);

#endif

#endif
