#ifndef __DIRENT_HPP__
#define __DIRENT_HPP__

#include <vfs.hpp>
#include <cpp.hpp>

cextern typedef struct {
	fs_node_t *base;
	unsigned long next_index;
} DIR;

cextern DIR *opendir(const char *name);
cextern struct dirent *readdir(DIR *dir);
cextern int closedir(DIR *dir);

#endif

