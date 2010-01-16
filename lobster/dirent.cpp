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

#include <dirent.hpp>
#include <memory.hpp>
#include <string.hpp>

DIR *opendir(const char *name) {
	fs_node_t *base = fs_root;

	DIR *new_dir = (DIR *)kmalloc(sizeof(DIR));
	new_dir->base = base;
	new_dir->next_index = 0;
	
	return new_dir;
}

struct dirent *readdir(DIR *dir) {
	static struct dirent dirent;

	dirent_t *node = readdir_fs(dir->base, dir->next_index);
	if (node == 0)
		return 0;
	dir->next_index++;

	strcpy(dirent.name, node->name);
	dirent.ino = node->ino;

	return &dirent;
}

int closedir(DIR *dir) {
	kfree(dir);
	return 0;
}

