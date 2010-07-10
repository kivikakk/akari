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

extern "C" int main() {
	std::string cwd = "/";
	
	while (true) {
		printf("%s@%s:%s%s ", "celtic", "akari", cwd.c_str(), "#");

		std::vector<std::string> line = getline().split();

		if (line.size() == 0) continue;

		if (line[0] == "ls") {
			DIR *dirp = opendir(cwd.c_str());
			VFSDirent *dirent;

			int entries = 0;

			while ((dirent = readdir(dirp))) {
				printf("%d\t%s\n", dirent->inode, dirent->name);
				++entries;
			}

			closedir(dirp);

			printf("%d %s\n", entries, entries == 1 ? "entry" : "entries");
		} else if (line[0] == "cd") {
			std::string new_cwd = cwd;

			if (line[1] == ".")
				continue;
			else if (line[1] == "..") {
				if (cwd == "/") continue;

				new_cwd = new_cwd.substr(0, new_cwd.rfind('/'));
				if (new_cwd.length() == 0)
					new_cwd = "/";
			} else if (line[1][0] == '/') {
				new_cwd = line[1];
			} else {
				if (cwd.length() != 1)
					new_cwd += '/';
				new_cwd += line[1];
			}

			DIR *dirp = opendir(new_cwd.c_str());
			if (!dirp) {
				printf("cd: %s: Filen eller katalogen finns inte\n", line[1].c_str());
			} else {
				closedir(dirp);
				cwd = new_cwd;
			}
		} else {
			printf("%s: command not found\n", line[0].c_str());
		}
	}
}
