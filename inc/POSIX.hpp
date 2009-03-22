#ifndef __POSIX_HPP__
#define __POSIX_HPP__

#include <arch.hpp>

namespace POSIX {
	extern "C" void *memset(void *, u8, u32);
	extern "C" void *memcpy(void *, const void *, u32);
}

#endif

