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

#include <stdio.hpp>
#include <fs.hpp>
#include <UserCalls.hpp>
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>

#include "../VFS/VFSProto.hpp"

void lobster();

extern "C" int main() {
	const char *cwd = "/";
	
	while (true) {
		printf("(Akari) %s$ ", cwd);

		std::vector<std::string> line = getline().split();

		if (line.size() == 0) continue;

		if (line[0] == "ls") {

			DIR *dirp = opendir(cwd);
			VFSDirent *dirent;
			u32 i = 0; bool started = false;
			while ((dirent = readdir(dirp))) {
				if (i == 0) {
					if (!started)
						started = true;
					else
						printf("\n");
				}
				printf("%s", dirent->name);
				i = (i + 1) % 6;
				if (i != 0) printf("\t");
			}

			printf("\n");

			closedir(dirp);

		} else if (line[0] == "lobster") {
			lobster();
		} else {
			printf("%s: command not found\n", line[0].c_str());
		}
	}

	panic("shell exited?");
	return 1;
}

void lobster() {
	printf(
		"                         ,.---.\n"
		"               ,,,,     /    _ `.\n"
		"                \\\\\\\\   /      \\  )\n"
		"                 |||| /\\/``-.__\\/\n"
		"                 ::::/\\/_\n"
		" {{`-.__.-'(`(^^(^^^(^ 0 `.========='\n"
		"{{{{{{ { ( ( (  (   (-----:=\n"
		" {{.-'~~'-.(,(,,(,,,(__0_.'=========.\n"
		"                 ::::\\/\\ \n"
		"                 |||| \\/\\  ,-'/\\\n"
		"                ////   \\ `` _/  )\n"
		"               ''''     \\  `   /\n"
		"                         `---''\n");
}

