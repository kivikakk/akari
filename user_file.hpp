#ifndef __USER_FILE_HPP__
#define __USER_FILE_HPP__

#include <cpp.hpp>

cextern int user_open(const char *path, int oflag);
cextern int user_read(int fildes, void *buf, unsigned long nbyte);
cextern int user_close(int fildes);

#endif

