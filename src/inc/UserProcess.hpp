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

#ifndef __USER_PROCESS_HPP__
#define __USER_PROCESS_HPP__

#include <arch.hpp>
#include <UserGates.hpp>

#if defined(__AKARI_KERNEL)

namespace User {
namespace Process {
	pid_t fork();
	pid_t spawn(const char *name, const u8 *elf, u32 elf_len, char *const *args);
	bool grantPrivilege(pid_t task, u16 priv);
	bool grantIOPriv(pid_t task, u16 port);
	bool beginExecution(pid_t task);
}
}

#elif defined(__AKARI_LINKAGE)

DEFN_SYSCALL0(fork, 35, pid_t)
DEFN_SYSCALL4(spawn, 36, pid_t, const char *, const u8 *, u32, char *const *)
DEFN_SYSCALL2(grantPrivilege, 45, bool, pid_t, u16)
DEFN_SYSCALL2(grantIOPriv, 46, bool, pid_t, u16)
DEFN_SYSCALL1(beginExecution, 47, bool, pid_t)

#else

DECL_SYSCALL0(fork, pid_t);
DECL_SYSCALL4(spawn, pid_t, const char *, const u8 *, u32, char *const *);
DECL_SYSCALL2(grantPrivilege, bool, pid_t, u16);
DECL_SYSCALL2(grantIOPriv, bool, pid_t, u16);
DECL_SYSCALL1(beginExecution, bool, pid_t);

#endif

#endif

