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

#ifndef __FS_HPP
#define __FS_HPP

#include "../packages/VFS/VFSProto.hpp"

typedef struct {
	VFSNode *file;
	u32 offset;
} FILE;

FILE *fopen(const char *filename, const char *mode);
u32 fread(void *buf, u32 size, u32 n, FILE *stream);
u32 flen(FILE *stream);
int fclose(FILE *stream);

typedef struct {
	VFSNode *dir;
	u32 index;
} DIR;

DIR *opendir(const char *dirname);
VFSDirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

#endif

