#ifndef __POSIX_HPP__
#define __POSIX_HPP__

#include <arch.hpp>

namespace POSIX {
	extern "C" void *memset(void *, u8, u32);
	extern "C" void *memcpy(void *, const void *, u32);

	extern "C" u32 strlen(const char *);
	extern "C" s32 strcmp(const char *s1, const char *s2);
	extern "C" char *strcpy(char *dest, const char *src);
}

#endif

