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
#include <proc.hpp>
#include <fs.hpp>
#include <time.hpp>
#include <UserCalls.hpp>
#include <UserIPC.hpp>
#include <UserIPCQueue.hpp>

#include "../vfs/proto.hpp"

std::string join(const std::string &a, const std::string &b) {
	if (a.length() == 0)
		return b;
	if (a[a.length() - 1] == '/')
		return a + b;
	return a + '/' + b;
}

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
			std::string path;

			if (line[1] == ".")
				continue;
			else if (line[1] == "..") {
				if (cwd == "/") continue;

				path = cwd.substr(0, cwd.rfind('/'));
			} else if (line[1][0] == '/') {
				path = line[1];
			} else {
				path = join(cwd, line[1]);
			}

			if (path.length() == 0)
				path = '/';
			else if (path.length() > 1 && path[path.length() - 1] == '/')
				path = path.substr(0, path.length() - 1);

			DIR *dirp = opendir(path.c_str());
			if (!dirp) {
				printf("cd: %s: No such file or directory\n", line[1].c_str());
			} else {
				closedir(dirp);
				cwd = path;
			}
		} else if (line[0] == "usleep") {
			usleep(atoi(line[1].c_str()));
		} else {
			std::string path;
		   
			if (line[0][0] == '/') {
				path = line[0];
			} else {
				path = join(cwd, line[0]);
			}

			bool bg = false;
			if (path[path.length() - 1] == '&') {
				bg = true;
				path = path.substr(0, path.length() - 1);
			}

			if (fexists(path.c_str())) {
				if (fdir(path.c_str())) {
					printf("%s: is a directory\n", line[0].c_str());
				} else {
					pid_t pid = bootstrap(path.c_str(), line);
					if (!bg)
						waitProcess(pid);
				}
			} else {
				printf("%s: No such file or directory\n", line[0].c_str());
			}
		}
	}
}
