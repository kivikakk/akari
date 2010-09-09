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

#ifndef __PROC_HPP
#define __PROC_HPP

#include <arch.hpp>
#include <string>
#include <slist>

typedef class BootstrapOptions {
public:
	BootstrapOptions(): privs(), iobits() {
	}

	std::slist<u32> privs;
	std::slist<u16> iobits;
} bootstrap_options_t;

pid_t bootstrap(const char *filename, const std::slist<std::string> &args=std::slist<std::string>(), const bootstrap_options_t &options=bootstrap_options_t());

#endif

