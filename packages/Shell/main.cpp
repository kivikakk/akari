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
		printf("%s@%s:%s%s ", "celtic", "akari", cwd, "#");

		std::vector<std::string> line = getline().split();

		if (line.size() == 0) continue;

		if (line[0] == "ls") {

			DIR *dirp = opendir(cwd);
			VFSDirent *dirent;

			int entries = 0;

			while ((dirent = readdir(dirp))) {
				printf("%x\t%s\n", dirent->inode, dirent->name);
				++entries;
			}

			closedir(dirp);

			printf("%d %s\n", entries, entries == 1 ? "entry" : "entries");

		} else {
			printf("%s: command not found\n", line[0].c_str());
		}
	}
}
