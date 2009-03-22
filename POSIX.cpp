#include <POSIX.hpp>

void *POSIX::memset(void *mem, u8 c, u32 n) {
	u8 *m = (u8 *)mem;
	while (n--)
		*m++ = c;
	return mem;
}

