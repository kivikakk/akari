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

#include <UserProcess.hpp>
#include <debug.hpp>

#if defined(__AKARI_KERNEL)

namespace User {
namespace Process {
	pid_t fork() {
		// Copy ourselves and set us running!
		AkariPanic("The more I reflect on it, the more I realise fork is really challenging. TODO.");
		return 0;
	}

	pid_t spawn(const char *name, const u8 *elf, u32 elf_len) {

		return 0;
	}
}
}
#endif
