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

pid_t bootstrap(const char *filename, const std::slist<std::string> &args, const bootstrap_options_t &options) {
	FILE *prog = fopen(filename, "r");
	if (!prog) {
		printf("couldn't open %s\n", filename);
		panic("!prog");
	}

	u32 image_len = flen(prog);
	u8 *image = new u8[image_len];
	fread(image, image_len, 1, prog);
	fclose(prog);

	char **argsc = new char*[args.size() + 1];
	u32 i = 0;
	for (std::slist<std::string>::iterator it = args.begin(); it != args.end(); ++it) {
		argsc[i++] = strdup(it->c_str());
	}
	argsc[i] = 0;

	pid_t task = spawn(filename, image, image_len, argsc);

	for (i = 0; i < args.size(); ++i)
		delete [] argsc[i];
	delete [] argsc;
	delete [] image;

	for (std::slist<u32>::iterator it = options.privs.begin(); it != options.privs.end(); ++it)
		grantPrivilege(task, *it);

	for (std::slist<u16>::iterator it = options.iobits.begin(); it != options.iobits.end(); ++it)
		grantIOPriv(task, *it);


	beginExecution(task);

	return task;
}

