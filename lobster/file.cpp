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

#include <file.hpp>
#include <vfs.hpp>
#include <memory.hpp>

FILE *fopen(const char *path, const char *mode /* ignored for now */) {
	FILE *new_file = (FILE *)kmalloc(sizeof(FILE));

	new_file->node = resolve_fs_node(path);
	new_file->offset = 0;
	new_file->at_eof = 1;

	open_fs(new_file->node, 1, 0);	/* flags not used anyway? */

	return new_file;
}

unsigned long fread(void *ptr, unsigned long size, unsigned long nmemb, FILE *file) {
	unsigned long returned = read_fs(file->node, file->offset, size * nmemb, (unsigned char *)ptr);
	if (returned < size * nmemb)
		file->at_eof = 1;		// naive but who cares?
	return returned / size;
}

int feof(FILE *file) {
	if (file->at_eof)
		return file->at_eof;
	return (file->at_eof = !(file->offset < file->node->length));
}

// this function isn't in POSIX. sorry.
unsigned long flen(FILE *file) {
	return file->node->length;
}

int fclose(FILE *file) {
	close_fs(file->node);

	kfree(file);
	return 0;
}

