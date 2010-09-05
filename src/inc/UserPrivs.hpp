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

#ifndef __USER_PRIVS_HPP__
#define __USER_PRIVS_HPP__

#include <arch.hpp>

// Update PRIV_COUNT when this changes.
typedef enum {
	PRIV_GRANT_PRIV = 0,
	PRIV_CONSOLE_WRITE = 1,
	PRIV_IRQ = 2,
	PRIV_MALLOC = 3,
	PRIV_PHYSADDR = 4,
	PRIV_PAGING_ADMIN = 5,
	PRIV_REGISTER_NAME = 6
} priv_t;

#define PRIV_COUNT 7
#define PRIV_MAP_SIZE ((PRIV_COUNT + 7) / 8)

namespace User {
	void requirePrivilege(priv_t priv);
}

#endif

