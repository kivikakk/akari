#ifndef __FILE_HPP__
#define __FILE_HPP__

#include <vfs.hpp>
#include <cpp.hpp>

cextern typedef struct {
	fs_node_t *node;
	unsigned long offset;
	int at_eof;
} FILE;

cextern FILE *fopen(const char *path, const char *mode);
cextern unsigned long fread(void *ptr, unsigned long size, unsigned long nmemb, FILE *file);
cextern int feof(FILE *file);
cextern unsigned long flen(FILE *file);
cextern int fclose(FILE *);

#endif

