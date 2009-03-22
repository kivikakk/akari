#include <POSIX.hpp>
#include <Akari.hpp>

namespace POSIX {
	void *memset(void *mem, u8 c, u32 n) {
		u8 *m = (u8 *)mem;
		while (n--)
			*m++ = c;
		return mem;
	}

	void *memcpy(void *dest, const void *src, u32 n) {
		const u8 *s = (const u8 *)src;
		u8 *d = (u8 *)dest;
		while (n--)
			*d++ = *s++;
		return dest;
	}
}
