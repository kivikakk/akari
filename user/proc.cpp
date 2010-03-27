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

#include <proc.hpp>
#include <stdio.hpp>
#include <fs.hpp>
#include <UserProcess.hpp>
#include <UserCalls.hpp>

pid_t bootstrap(const char *filename) {
	FILE *prog = fopen(filename, "r");
	if (!prog) {
		printf("couldn't open %s\n", filename);
		panic("!prog");
	}

	u32 image_len = flen(prog);
	u8 *image = new u8[image_len];
	fread(image, image_len, 1, prog);
	fclose(prog);

	pid_t pid = spawn(filename, image, image_len);
	delete [] image;
	return pid;
}

