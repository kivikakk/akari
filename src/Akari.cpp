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

#include <Akari.hpp>
#include <Memory.hpp>
#include <Console.hpp>
#include <Descriptor.hpp>
#include <debug.hpp>

Memory *mu_memory = 0;
Console *mu_console = 0;
Descriptor *mu_descriptor = 0;
Timer *mu_timer = 0;
Tasks *mu_tasks = 0;
Syscall *mu_syscall = 0;
ELF *mu_elf = 0;
Debugger *mu_debugger = 0;
